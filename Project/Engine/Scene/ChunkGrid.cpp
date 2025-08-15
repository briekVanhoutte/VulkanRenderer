#include "ChunkGrid.h"

#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>

#include <Engine/Scene/GameObjects/BaseObject.h>
#include <Engine/ObjUtils/DebugPrint.h>

float ChunkGrid::s_defaultChunkSize = 32.f;
void ChunkGrid::SetDefaultChunkSize(float s) {
    s_defaultChunkSize = (s > 0.0f) ? s : s_defaultChunkSize;
}
float ChunkGrid::DefaultChunkSize() {
    return s_defaultChunkSize;
}

glm::vec3 ChunkGrid::MinCornerOf(const ChunkCoord& c) {
    const float hs = s_defaultChunkSize * 0.5f;
    const glm::vec3 center{
        (c.x + 0.5f) * s_defaultChunkSize,
        (c.y + 0.5f) * s_defaultChunkSize,
        (c.z + 0.5f) * s_defaultChunkSize
    };
    return { center.x - hs, 0.0f, center.z - hs }; // ground plane (y = 0)
}
glm::vec3 ChunkGrid::MaxCornerOf(const ChunkCoord& c) {
    const float hs = s_defaultChunkSize * 0.5f;
    const glm::vec3 center{
        (c.x + 0.5f) * s_defaultChunkSize,
        (c.y + 0.5f) * s_defaultChunkSize,
        (c.z + 0.5f) * s_defaultChunkSize
    };
    return { center.x + hs, 0.0f, center.z + hs }; // ground plane (y = 0)
}
// ---------- debug stringify ----------
static std::string to_string(const ChunkCoord& c) {
    std::ostringstream ss; ss << "(" << c.x << "," << c.y << "," << c.z << ")"; return ss.str();
}
static std::string to_string(const MeshKey& k) {
    std::ostringstream ss;
    ss << "{VB=" << ptrStr((void*)k.vertexBuffer)
        << "+" << k.vbOffset
        << ", IB=" << ptrStr((void*)k.indexBuffer)
        << "+" << k.ibOffset
        << ", idx=" << k.indexCount
        << ", pipe=" << k.pipelineIndex
        << ", group=" << k.logicalId
        << "}";
    return ss.str();
}

// ---------- private statics ----------
glm::vec3 ChunkGrid::getPos(const BaseObject* obj) {
    return const_cast<BaseObject*>(obj)->getPosition(); // your getter is non-const
}
void ChunkGrid::forVisibleCells(const CellCallback& fn) const {
    auto emit = [&](const ChunkCoord& c, bool forceVisible) {
        if (!forceVisible) {
            if (!chunkWithinRadius2D(c)) return;
            if (!passFrontCone(c))      return;
        }

        // Use this instance's chunk size for AABB, but place on ground plane (y=0)
        const glm::vec3 ctr = fromChunk(c);
        const float hs = m_chunkSize * 0.5f;
        const glm::vec3 mn{ ctr.x - hs, 0.0f, ctr.z - hs };
        const glm::vec3 mx{ ctr.x + hs, 0.0f, ctr.z + hs };
        fn(c, mn, mx);
        };

    // 1) camera chunk first
    const ChunkCoord camC = toChunk(m_camPos);
    emit(camC, /*forceVisible*/ true);

    // 2) neighborhood in XZ within render distance
    const glm::vec3 rvec(m_renderDistance);
    const glm::vec3 pmin = m_camPos - rvec;
    const glm::vec3 pmax = m_camPos + rvec;

    const ChunkCoord minC = toChunk({ pmin.x, 0, pmin.z });
    const ChunkCoord maxC = toChunk({ pmax.x, 0, pmax.z });

    for (int z = minC.z; z <= maxC.z; ++z)
        for (int x = minC.x; x <= maxC.x; ++x) {
            ChunkCoord c{ x, 0, z };
            if (c.x == camC.x && c.y == camC.y && c.z == camC.z) continue;
            emit(c, /*forceVisible*/ false);
        }
}
// ---------- culling config ----------
void ChunkGrid::setCulling(const glm::vec3& camPos, float renderDistance,
    const glm::vec3& camForward,
    bool use2D,
    float frontConeDegrees,
    bool /*useCenterTestIgnored*/,
    bool invertForward)
{
    m_camPos = camPos;
    m_renderDistance = renderDistance;
    m_use2D = use2D;
    m_invertForward = invertForward;

    // normalize forward if valid
    const float f2 = camForward.x * camForward.x + camForward.y * camForward.y + camForward.z * camForward.z;
    m_camForward = (f2 > 0.0f) ? glm::normalize(camForward) : glm::vec3(0, 0, 1);
    if (m_invertForward) m_camForward = -m_camForward;

    // store cos(half-angle); negative disables
    if (frontConeDegrees > 0.0f) {
        const float halfRad = 0.5f * frontConeDegrees * (3.14159265358979323846f / 180.0f);
        m_frontConeCos = std::cos(halfRad);
    }
    else {
        m_frontConeCos = -1.0f;
    }

    m_chunkRadius = (int)std::ceil(renderDistance / m_chunkSize);
}

// ---------- addressing ----------
ChunkCoord ChunkGrid::toChunk(const glm::vec3& p) const {
    if (m_use2D) {
        return ChunkCoord{
            (int)std::floor(p.x / m_chunkSize),
            0,
            (int)std::floor(p.z / m_chunkSize)
        };
    }
    return ChunkCoord{
        (int)std::floor(p.x / m_chunkSize),
        (int)std::floor(p.y / m_chunkSize),
        (int)std::floor(p.z / m_chunkSize)
    };
}

glm::vec3 ChunkGrid::fromChunk(const ChunkCoord& c) const {
    return { (c.x + 0.5f) * m_chunkSize,
             (c.y + 0.5f) * m_chunkSize,
             (c.z + 0.5f) * m_chunkSize };
}

// ---------- membership ops ----------
void ChunkGrid::add(BaseObject* obj, const MeshKey& key) {
    (void)m_spatial[obj]; // ensure default SpatialInfo
    insertAccordingToSpatial(obj, key);
}

void ChunkGrid::remove(BaseObject* obj) {
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
                auto& mh = m_handles[moved];
                for (auto& mref : mh.perChunkRefs) {
                    if (mref.chunk.x == ref.chunk.x &&
                        mref.chunk.y == ref.chunk.y &&
                        mref.chunk.z == ref.chunk.z) {
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

void ChunkGrid::update(BaseObject* obj, const MeshKey& newKey) {
    auto it = m_handles.find(obj);
    if (it == m_handles.end()) { add(obj, newKey); return; }
    remove(obj);
    insertAccordingToSpatial(obj, newKey);
}

// ---------- coverage ----------
void ChunkGrid::setCoverageOverride(BaseObject* obj, const glm::vec3& center, const glm::vec3& halfExtents) {
    SpatialInfo& s = m_spatial[obj];
    s.overrideEnabled = true;
    s.overrideCenter = center;
    s.overrideHalfExtents = halfExtents;
    auto it = m_handles.find(obj);
    if (it != m_handles.end()) update(obj, it->second.key);
}
void ChunkGrid::clearCoverageOverride(BaseObject* obj) {
    SpatialInfo& s = m_spatial[obj];
    if (!s.overrideEnabled) return;
    s.overrideEnabled = false;
    auto it = m_handles.find(obj);
    if (it != m_handles.end()) update(obj, it->second.key);
}

// ---------- tagging ----------
void ChunkGrid::setGlobal(BaseObject* obj, bool enable) {
    SpatialInfo& s = m_spatial[obj];
    s.tag = enable ? SpatialTag::Global : SpatialTag::SingleChunk;
    auto it = m_handles.find(obj);
    if (it != m_handles.end()) {
        MeshKey k = it->second.key;
        update(obj, k);
    }
}
void ChunkGrid::setMultiChunk(BaseObject* obj, const glm::vec3& halfExtents) {
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

// ---------- insertion ----------
void ChunkGrid::insertAccordingToSpatial(BaseObject* obj, const MeshKey& key) {
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

    glm::vec3 C, he;
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

    for (int z = minC.z; z <= maxC.z; ++z)
        for (int x = minC.x; x <= maxC.x; ++x) {
            ChunkCoord c{ x, 0, z };
            auto& vec = m_chunks[c].batches[key];
            vec.push_back(obj);
            handle.perChunkRefs.push_back(BatchRef{ c, vec.size() - 1 });
        }

    handle.isGlobal = false;
    m_handles[obj] = std::move(handle);
}

// ---------- visibility helpers ----------
bool ChunkGrid::chunkWithinRadius2D(const ChunkCoord& c) const {
    // STRICT linear distance in XZ to chunk CENTER, no Y and no extra padding.
    glm::vec3 ctr = fromChunk(c);
    float dx = ctr.x - m_camPos.x;
    float dz = ctr.z - m_camPos.z;
    return (dx * dx + dz * dz) <= (m_renderDistance * m_renderDistance);
}

bool ChunkGrid::passFrontCone(const ChunkCoord& c) const {
    if (m_frontConeCos < -0.5f) return true; // disabled

    glm::vec3 ctr = fromChunk(c);
    glm::vec3 to = m_use2D ? glm::vec3(ctr.x - m_camPos.x, 0, ctr.z - m_camPos.z)
        : (ctr - m_camPos);

    glm::vec3 fwd = m_camForward;
    if (m_use2D) fwd = glm::vec3(fwd.x, 0, fwd.z);

    float toLen2 = to.x * to.x + to.y * to.y + to.z * to.z;
    float fwdLen2 = fwd.x * fwd.x + fwd.y * fwd.y + fwd.z * fwd.z;

    if (toLen2 < 1e-8f || fwdLen2 < 1e-8f) return true;

    float d = (to.x * fwd.x + to.y * fwd.y + to.z * fwd.z) / std::sqrt(toLen2 * fwdLen2);
    return d >= m_frontConeCos;
}

// ---------- debug ----------
void ChunkGrid::debugPrintStorage(std::ostream& os) const {
    size_t totalObjs = 0, totalBatches = 0;
    os << "=== ChunkGrid storage ===\n";
    os << "chunks=" << m_chunks.size() << "\n";

    for (const auto& kvChunk : m_chunks) {
        const ChunkCoord& cc = kvChunk.first;
        const Chunk& ch = kvChunk.second;
        os << "Chunk " << to_string(cc) << " : " << ch.batches.size() << " batch(es)\n";
        for (const auto& kvBatch : ch.batches) {
            const MeshKey& key = kvBatch.first;
            const auto& vec = kvBatch.second;
            os << "  Key " << to_string(key) << " -> " << vec.size() << " obj(s)\n";
            totalBatches++;
            totalObjs += vec.size();
#ifdef DEBUG_CHUNKGRID_VERBOSE
            for (const BaseObject* o : vec) {
                auto pos = const_cast<BaseObject*>(o)->getPosition();
                os << "    obj " << o << " pos=(" << pos.x << "," << pos.y << "," << pos.z << ")\n";
            }
#endif
        }
    }

    os << "Global batches: " << m_globalBatches.size() << "\n";
    for (const auto& kv : m_globalBatches) {
        const MeshKey& key = kv.first;
        const auto& vec = kv.second;
        os << "  [Global] Key " << to_string(key) << " -> " << vec.size() << " obj(s)\n";
        totalBatches++;
        totalObjs += vec.size();
    }

    os << "Totals: objs=" << totalObjs
        << ", batches=" << totalBatches
        << ", chunks=" << m_chunks.size() << "\n";
}

void ChunkGrid::debugPrintObjectPlacement(std::ostream& os) const {
    os << "=== Object placement (" << m_handles.size() << " objects) ===\n";
    for (const auto& kv : m_handles) {
        const BaseObject* obj = kv.first;
        const ObjectHandle& h = kv.second;
        os << "Obj " << obj;
        if (h.isGlobal) {
            os << " GLOBAL key=" << to_string(h.key) << "\n";
        }
        else {
            os << " in " << h.perChunkRefs.size() << " chunk(s): ";
            for (const BatchRef& r : h.perChunkRefs)
                os << to_string(r.chunk) << " ";
            os << " | key=" << to_string(h.key) << "\n";
        }
    }
}
