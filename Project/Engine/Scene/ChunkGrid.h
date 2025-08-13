#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <glm/glm.hpp>
#include "MeshKeyUtil.h"


struct ChunkCoord { int x, y, z; };
inline bool operator==(const ChunkCoord& a, const ChunkCoord& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
struct ChunkCoordHash {
    size_t operator()(const ChunkCoord& c) const noexcept {
        size_t h = 1469598103934665603ull;
        auto mix = [&](int v) { h ^= (size_t)v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2); };
        mix(c.x); mix(c.y); mix(c.z); return h;
    }
};

//class BaseObject;


struct Chunk {
    std::unordered_map<MeshKey, std::vector<BaseObject*>, MeshKeyHash> batches;
};

enum class SpatialTag : uint8_t { SingleChunk, MultiChunk, Global };

struct SpatialInfo {
    SpatialTag tag = SpatialTag::SingleChunk;
    glm::vec3  halfExtents{ 0.0f }; // for MultiChunk, AABB half-extents in world space

    bool       overrideEnabled = false;
    glm::vec3  overrideCenter{ 0.0f };
    glm::vec3  overrideHalfExtents{ 0.0f };
};

struct BatchRef {
    ChunkCoord chunk{};
    size_t     indexInBatch{};
};

// NOTE: This is the ONLY ObjectHandle definition
struct ObjectHandle {
    MeshKey key{};
    bool    isGlobal = false;
    std::vector<BatchRef> perChunkRefs; // empty for global
    size_t  globalIndex = SIZE_MAX;     // valid only if isGlobal==true
};



class ChunkGrid {
public:
    explicit ChunkGrid(float chunkSize = 32.f) : m_chunkSize(chunkSize) {}

    void setCulling(const glm::vec3& camPos, float renderDistance) {
        m_camPos = camPos; m_renderDistance = renderDistance;
        m_chunkRadius = (int)glm::ceil(renderDistance / m_chunkSize);
    }

    void add(BaseObject* obj, const MeshKey& key) {
        // If spatial info not set yet, default SingleChunk
        (void)m_spatial[obj];
        insertAccordingToSpatial(obj, key);
    }

    void remove(BaseObject* obj) {
        auto it = m_handles.find(obj);
        if (it == m_handles.end()) return;

        ObjectHandle h = it->second;
        if (h.isGlobal) {
            auto gIt = m_globalBatches.find(h.key);
            if (gIt != m_globalBatches.end()) {
                auto& vec = gIt->second;
                size_t last = vec.size() - 1;
                std::swap(vec[h.globalIndex], vec[last]);
                BaseObject* moved = vec[h.globalIndex];
                if (moved) m_handles[moved].globalIndex = h.globalIndex;
                vec.pop_back();
                if (vec.empty()) m_globalBatches.erase(gIt);
            }
        }
        else {
            // Remove from every chunk batch we’re in
            for (const BatchRef& ref : h.perChunkRefs) {
                auto chIt = m_chunks.find(ref.chunk);
                if (chIt == m_chunks.end()) continue;
                auto bIt = chIt->second.batches.find(h.key);
                if (bIt == chIt->second.batches.end()) continue;
                auto& vec = bIt->second;
                size_t last = vec.size() - 1;
                std::swap(vec[ref.indexInBatch], vec[last]);
                BaseObject* moved = vec[ref.indexInBatch];
                if (moved) {
                    // Fix moved element’s BatchRef pointing to this same chunk/key
                    auto& mh = m_handles[moved];
                    for (auto& mref : mh.perChunkRefs) {
                        if (mref.chunk.x == ref.chunk.x && mref.chunk.y == ref.chunk.y && mref.chunk.z == ref.chunk.z) {
                            mref.indexInBatch = ref.indexInBatch;
                            break;
                        }
                    }
                }
                vec.pop_back();
                if (vec.empty()) chIt->second.batches.erase(h.key);
                if (chIt->second.batches.empty()) m_chunks.erase(chIt);
            }
        }
        m_handles.erase(it);
    }

    // call when obj moved or changed material/mesh
    void update(BaseObject* obj, const MeshKey& newKey) {
        auto it = m_handles.find(obj);
        if (it == m_handles.end()) { add(obj, newKey); return; }
        // Simple + safe: remove + reinsert (keeps logic centralized)
        remove(obj);
        insertAccordingToSpatial(obj, newKey);
    }

    void setCoverageOverride(BaseObject* obj, const glm::vec3& center, const glm::vec3& halfExtents) {
        SpatialInfo& s = m_spatial[obj];
        s.overrideEnabled = true;
        s.overrideCenter = center;
        s.overrideHalfExtents = halfExtents;
        auto it = m_handles.find(obj);
        if (it != m_handles.end()) update(obj, it->second.key); // reinsert with new coverage
    }

    // Remove coverage override (falls back to Single/Multi behavior)
    void clearCoverageOverride(BaseObject* obj) {
        SpatialInfo& s = m_spatial[obj];
        if (!s.overrideEnabled) return;
        s.overrideEnabled = false;
        auto it = m_handles.find(obj);
        if (it != m_handles.end()) update(obj, it->second.key);
    }

    template<typename Fn>
    void forVisibleBatches(Fn&& fn) {
        std::unordered_set<BaseObject*> visited;

        // visible chunks
        ChunkCoord cc = toChunk(m_camPos);
        for (int z = -m_chunkRadius; z <= m_chunkRadius; ++z)
            for (int y = -m_chunkRadius; y <= m_chunkRadius; ++y)
                for (int x = -m_chunkRadius; x <= m_chunkRadius; ++x) {
                    ChunkCoord c{ cc.x + x, cc.y + y, cc.z + z };
                    auto it = m_chunks.find(c);
                    if (it == m_chunks.end()) continue;

                    // sphere-ish cutoff
                    glm::vec3 center = fromChunk(c);
                    if (glm::distance(center, m_camPos) > (m_chunkRadius + 0.5f) * m_chunkSize) continue;

                    for (auto& kv : it->second.batches) {
                        const MeshKey& key = kv.first;
                        const auto& vec = kv.second;
                        if (vec.empty()) continue;

                        // filter out duplicates (multi-chunk objects may appear in several chunks)
                        std::vector<BaseObject*> unique;
                        unique.reserve(vec.size());
                        for (BaseObject* o : vec) {
                            if (visited.insert(o).second) unique.push_back(o);
                        }
                        if (!unique.empty()) fn(key, unique);
                    }
                }

        // always-visible globals (no need to dedup: they are not in chunks)
        for (auto& kv : m_globalBatches) {
            if (!kv.second.empty()) fn(kv.first, kv.second);
        }
    }

    float chunkSize() const { return m_chunkSize; }

    void setGlobal(BaseObject* obj, bool enable) {
        SpatialInfo& s = m_spatial[obj];
        s.tag = enable ? SpatialTag::Global : SpatialTag::SingleChunk;
        // If already registered, reinsert with new behavior
        auto it = m_handles.find(obj);
        if (it != m_handles.end()) {
            MeshKey k = it->second.key;
            update(obj, k);
        }
    }

    // Mark as multi-chunk with world half-extents (axis-aligned); set (0,0,0) to revert to single-chunk
    void setMultiChunk(BaseObject* obj, const glm::vec3& halfExtents) {
        SpatialInfo& s = m_spatial[obj];
        s.halfExtents = halfExtents;
        s.tag = (glm::any(glm::greaterThan(halfExtents, glm::vec3(0.0f))))
            ? SpatialTag::MultiChunk
            : SpatialTag::SingleChunk;
        auto it = m_handles.find(obj);
        if (it != m_handles.end()) {
            MeshKey k = it->second.key;
            update(obj, k);
        }
    }

private:
    // You already have BaseObject::getPosition()
    static glm::vec3 getPos(const BaseObject* obj);

    // Insert based on SpatialInfo tag (used by add/update)
    void insertAccordingToSpatial(BaseObject* obj, const MeshKey& key) {
        const SpatialInfo& s = m_spatial[obj];
        ObjectHandle handle;
        handle.key = key;

        if (s.tag == SpatialTag::Global) {
            auto& vec = m_globalBatches[key];
            vec.push_back(obj);
            handle.isGlobal = true;
            handle.globalIndex = vec.size() - 1;
            m_handles[obj] = std::move(handle);
            return;
        }

        // Single or Multi: compute chunk list
        glm::vec3 C;
        glm::vec3 he;

        if (s.overrideEnabled) {
            C = s.overrideCenter;
            he = s.overrideHalfExtents;
        }
        else {
            C = getPos(obj);
            he = (s.tag == SpatialTag::MultiChunk) ? s.halfExtents : glm::vec3(0.0f);
        }
        glm::vec3 minP = C - he;
        glm::vec3 maxP = C + he;
        ChunkCoord minC = toChunk(minP);
        ChunkCoord maxC = toChunk(maxP);

        // clamp to reasonable range if needed; here we trust data
        for (int z = minC.z; z <= maxC.z; ++z)
            for (int y = minC.y; y <= maxC.y; ++y)
                for (int x = minC.x; x <= maxC.x; ++x) {
                    ChunkCoord c{ x,y,z };
                    auto& vec = m_chunks[c].batches[key];
                    vec.push_back(obj);
                    handle.perChunkRefs.push_back(BatchRef{ c, vec.size() - 1 });
                }

        handle.isGlobal = false;
        m_handles[obj] = std::move(handle);
    }

    ChunkCoord toChunk(const glm::vec3& p) const {
        return ChunkCoord{ (int)glm::floor(p.x / m_chunkSize),
                           (int)glm::floor(p.y / m_chunkSize),
                           (int)glm::floor(p.z / m_chunkSize) };
    }
    glm::vec3 fromChunk(const ChunkCoord& c) const {
        return { (c.x + 0.5f) * m_chunkSize, (c.y + 0.5f) * m_chunkSize, (c.z + 0.5f) * m_chunkSize };
    }


    float m_chunkSize{ 32.f };
    glm::vec3 m_camPos{ 0 };
    float m_renderDistance{ 128.f };
    int m_chunkRadius{ 4 };

    std::unordered_map<ChunkCoord, Chunk, ChunkCoordHash> m_chunks;
    std::unordered_map<MeshKey, std::vector<BaseObject*>, MeshKeyHash> m_globalBatches;
    std::unordered_map<BaseObject*, ObjectHandle> m_handles;
    std::unordered_map<BaseObject*, SpatialInfo>  m_spatial;
};
