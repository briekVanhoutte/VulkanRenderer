#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>
#include "MeshKeyUtil.h"
#include <functional>

class BaseObject;

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

struct Chunk {
    std::unordered_map<MeshKey, std::vector<BaseObject*>, MeshKeyHash> batches;
};

enum class SpatialTag : uint8_t { SingleChunk, MultiChunk, Global };

struct SpatialInfo {
    SpatialTag tag = SpatialTag::SingleChunk;
    glm::vec3  halfExtents{ 0.0f };

    bool       overrideEnabled = false;
    glm::vec3  overrideCenter{ 0.0f };
    glm::vec3  overrideHalfExtents{ 0.0f };
};

struct BatchRef {
    ChunkCoord chunk{};
    size_t     indexInBatch{};
};

struct ObjectHandle {
    MeshKey key{};
    bool    isGlobal = false;
    std::vector<BatchRef> perChunkRefs;
    size_t  globalIndex = SIZE_MAX;
};

class ChunkGrid {
public:
    explicit ChunkGrid(float chunkSize = 32.f) : m_chunkSize(chunkSize) {}

    // Legacy wrapper
    void setCulling(const glm::vec3& camPos, float renderDistance) {
        setCulling(camPos, renderDistance, m_camForward, /*use2D*/ true,
            /*frontConeDegrees*/ -1.0f, /*useCenterTest (ignored)*/ true);
    }

    // Extended controls (frontConeDegrees < 0 disables front gating)
    void setCulling(const glm::vec3& camPos, float renderDistance,
        const glm::vec3& camForward,
        bool use2D,
        float frontConeDegrees,
        bool /*useCenterTestIgnored*/,
        bool invertForward = false);

    static void   SetDefaultChunkSize(float s);
    static float  DefaultChunkSize();
    static glm::vec3 MinCornerOf(const ChunkCoord& c);  // world-space, y=0
    static glm::vec3 MaxCornerOf(const ChunkCoord& c);  // world-space, y=0
    using CellCallback = std::function<void(const ChunkCoord&,
        const glm::vec3& /*min*/,
        const glm::vec3& /*max*/)>;

    // Membership
    void add(BaseObject* obj, const MeshKey& key);
    void remove(BaseObject* obj);
    void update(BaseObject* obj, const MeshKey& newKey);

    // Coverage overrides
    void setCoverageOverride(BaseObject* obj, const glm::vec3& center, const glm::vec3& halfExtents);
    void clearCoverageOverride(BaseObject* obj);

    // Enumerate visible batches (always include camera chunk)
    template<typename Fn>
    void forVisibleBatches(Fn&& fn) {
        std::unordered_set<BaseObject*> visited;

        auto emitChunk = [&](const ChunkCoord& c, bool forceVisible) {
            auto it = m_chunks.find(c);
            if (it == m_chunks.end()) return;

            if (!forceVisible) {
                if (!chunkWithinRadius2D(c)) return;
                if (!passFrontCone(c)) return; // disable by passing frontConeDegrees < 0
            }

            for (auto& kv : it->second.batches) {
                const MeshKey& key = kv.first;
                const auto& vec = kv.second;
                if (vec.empty()) continue;

                std::vector<BaseObject*> unique;
                unique.reserve(vec.size());
                for (BaseObject* o : vec)
                    if (visited.insert(o).second) unique.push_back(o);

                if (!unique.empty()) fn(key, unique);
            }
            };

        // 1) Always include camera chunk
        ChunkCoord camC = toChunk(m_camPos);
        emitChunk(camC, /*forceVisible*/ true);

        // 2) Normal range
        glm::vec3 rvec(m_renderDistance);
        glm::vec3 pmin = m_camPos - rvec;
        glm::vec3 pmax = m_camPos + rvec;

        ChunkCoord minC = toChunk({ pmin.x, 0, pmin.z });
        ChunkCoord maxC = toChunk({ pmax.x, 0, pmax.z });

        for (int z = minC.z; z <= maxC.z; ++z)
            for (int x = minC.x; x <= maxC.x; ++x) {
                ChunkCoord c{ x, 0, z };
                if (c.x == camC.x && c.y == camC.y && c.z == camC.z) continue;
                emitChunk(c, /*forceVisible*/ false);
            }

        // 3) Globals
        for (auto& kv : m_globalBatches)
            if (!kv.second.empty()) fn(kv.first, kv.second);
    }

    float chunkSize() const { return m_chunkSize; }

    // Tagging
    void setGlobal(BaseObject* obj, bool enable);
    void setMultiChunk(BaseObject* obj, const glm::vec3& halfExtents);

    // Debug
    void debugPrintStorage(std::ostream& os) const;
    void debugPrintObjectPlacement(std::ostream& os) const;

    void forVisibleCells(const CellCallback& fn) const;

private:
    static glm::vec3 getPos(const BaseObject* obj);


    // Spatial insertion
    void insertAccordingToSpatial(BaseObject* obj, const MeshKey& key);

    // Addressing
    ChunkCoord toChunk(const glm::vec3& p) const;
    glm::vec3  fromChunk(const ChunkCoord& c) const;

    // Visibility helpers (ONLY linear XZ distance to center)
    bool chunkWithinRadius2D(const ChunkCoord& c) const;
    bool passFrontCone(const ChunkCoord& c) const;

    // Params
    static float s_defaultChunkSize;
    float    m_chunkSize{ 32.f };
    glm::vec3 m_camPos{ 0 };
    float     m_renderDistance{ 128.f };
    int       m_chunkRadius{ 4 };

    // Storage
    std::unordered_map<ChunkCoord, Chunk, ChunkCoordHash> m_chunks;
    std::unordered_map<MeshKey, std::vector<BaseObject*>, MeshKeyHash> m_globalBatches;
    std::unordered_map<BaseObject*, ObjectHandle> m_handles;
    std::unordered_map<BaseObject*, SpatialInfo>  m_spatial;

    // Culling config
    glm::vec3 m_camForward{ 0,0,1 };
    bool      m_use2D{ true };              // Y is ignored for mapping/culling
    float     m_frontConeCos{ -1.0f };      // <0 => disabled
    bool      m_invertForward{ false };
};
