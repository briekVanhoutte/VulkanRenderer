// ObjUtils.h
#pragma once

// Include your Vertex definition first:
#include <glm/glm.hpp>
#include "fast_obj.h"
#include <Engine/Graphics/Vertex.h>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <string>
#include <mutex>

namespace ObjUtils {

    // ---------------- Key types ----------------
    struct Triplet {
        uint32_t p, t, n; // 1-based indices from fast_obj (0 means missing)
        bool operator==(const Triplet& o) const { return p == o.p && t == o.t && n == o.n; }
    };
    struct TripletHash {
        size_t operator()(const Triplet& k) const noexcept {
            size_t h = 1469598103934665603ull;
            auto mix = [&](uint64_t x) { h ^= x + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2); };
            mix(k.p); mix(k.t); mix(k.n); return h;
        }
    };

    struct DebugTrip { uint32_t p, t, n; };

    // --------------- OBJ loader ----------------
    // flipV: set true if you need v = 1 - v.
    // flipWinding: swap i1<->i2 per triangle.
    // debug: runtime logging on/off.
    // dropDegenerate: drop zero-area triangles if true.
    inline bool ParseOBJ(const char* path,
        std::vector<Vertex>& outVertices,
        std::vector<uint32_t>& outIndices,
        bool flipV = false,
        bool flipWinding = false,
        bool debug = false,
        bool dropDegenerate = true)
    {
        // ---- cache key from normalized path + flags ----
        auto makeKey = [&](const char* p) -> uint64_t {
            static constexpr uint64_t FNV_OFFSET = 1469598103934665603ull;
            static constexpr uint64_t FNV_PRIME = 1099511628211ull;

            // normalize to lower case so paths differing only by case share
            std::string s = p ? std::string(p) : std::string();
            std::transform(s.begin(), s.end(), s.begin(),
                [](unsigned char c) { return (unsigned char)std::tolower(c); });

            uint64_t h = FNV_OFFSET;
            for (unsigned char c : s) { h ^= (uint64_t)c; h *= FNV_PRIME; }

            // fold flags (debug omitted on purpose: it doesn't change geometry)
            uint64_t flags = (flipV ? 1ull : 0ull) |
                (flipWinding ? 2ull : 0ull) |
                (dropDegenerate ? 4ull : 0ull);
            // mix flags into hash
            h ^= flags + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
            return h;
            };

        struct CacheEntry {
            std::vector<Vertex>   verts;
            std::vector<uint32_t> inds;
        };

        // one cache & mutex per process (function-local statics are OK in headers)
        static std::mutex s_cacheMtx;
        static std::unordered_map<uint64_t, CacheEntry> s_cache;

        const uint64_t key = makeKey(path);

        // ---- cache hit? ----
        {
            std::lock_guard<std::mutex> lock(s_cacheMtx);
            auto it = s_cache.find(key);
            if (it != s_cache.end()) {
                outVertices = it->second.verts;
                outIndices = it->second.inds;
                if (debug) {
                    std::cout << "\n--- ParseOBJ: \"" << (path ? path : "")
                        << "\" [cache hit] ---\n"
                        << "Vertices: " << outVertices.size()
                        << "  Indices: " << outIndices.size() << "\n";
                }
                return true;
            }
        }

        // ---- cache miss → load and build geometry ----
        fastObjMesh* m = fast_obj_read(path);
        if (!m) return false;

        if (debug) {
            std::cout.setf(std::ios::fixed);
            std::cout << std::setprecision(2)
                << "\n--- ParseOBJ: \"" << (path ? path : "") << "\" ---\n";
        }

        // Pre-pass stats
        {
            size_t triTotal = 0, idxTotal = 0, nTri = 0, nQuad = 0, nNgon = 0;
            for (unsigned f = 0; f < m->face_count; ++f) {
                unsigned fv = m->face_vertices[f];
                if (fv >= 3) {
                    triTotal += (fv - 2);
                    idxTotal += 3 * (fv - 2);
                    if (fv == 3) ++nTri; else if (fv == 4) ++nQuad; else if (fv > 4) ++nNgon;
                }
            }
            if (debug) {
                std::cout << "faces=" << m->face_count
                    << "  tris=" << triTotal
                    << "  indices=" << idxTotal
                    << "  (tri=" << nTri
                    << ", quad=" << nQuad
                    << ", ngon=" << nNgon << ")\n";
            }
        }

        outVertices.clear();
        outIndices.clear();
        outVertices.reserve(m->index_count); // upper bound

        std::unordered_map<Triplet, uint32_t, TripletHash> cache;
        cache.reserve(m->index_count);

        std::vector<DebugTrip> dbgTrip;
        if (debug) dbgTrip.reserve(m->index_count);

        // Emit vertex or reuse from cache
        auto emitVertex = [&](const fastObjIndex& ix) -> uint32_t {
            Triplet key{ ix.p, ix.t, ix.n };
            auto it = cache.find(key);
            if (it != cache.end()) return it->second;

            Vertex v{};
            v.color = glm::vec3(1);

            // Position
            if (ix.p) {
                const float* P = &m->positions[3 * ix.p];
                v.pos = glm::vec3(P[0], P[1], P[2]);
            }
            else {
                v.pos = glm::vec3(0);
            }

            // UV
            if (ix.t) {
                const float* T = &m->texcoords[2 * ix.t];
                v.texCoord = glm::vec2(T[0], flipV ? (1.0f - T[1]) : T[1]);
            }
            else {
                v.texCoord = glm::vec2(0);
            }

            // Normal
            if (ix.n) {
                const float* N = &m->normals[3 * ix.n];
                v.normal = glm::vec3(N[0], N[1], N[2]);
            }
            else {
                v.normal = glm::vec3(0, 1, 0);
            }

            uint32_t newIndex = static_cast<uint32_t>(outVertices.size());
            outVertices.push_back(v);
            cache.emplace(key, newIndex);

            if (debug) {
                dbgTrip.push_back({ ix.p, ix.t, ix.n });
                std::cout << "  [emit] i=" << newIndex
                    << "  p/t/n=" << ix.p << "/" << ix.t << "/" << ix.n
                    << "  pos=(" << v.pos.x << "," << v.pos.y << "," << v.pos.z << ")"
                    << "  uv=(" << v.texCoord.x << "," << v.texCoord.y << ")"
                    << "  n=(" << v.normal.x << "," << v.normal.y << "," << v.normal.z << ")\n";
            }
            return newIndex;
            };

        // Push triangle with degeneracy check
        auto pushTri = [&](uint32_t ia, uint32_t ib, uint32_t ic) {
            if (ia == ib || ib == ic || ia == ic) {
                if (debug) std::cerr << "[degenerate] duplicate indices: "
                    << ia << "," << ib << "," << ic << "\n";
                if (dropDegenerate) return;
            }

            glm::vec3 A = outVertices[ia].pos;
            glm::vec3 B = outVertices[ib].pos;
            glm::vec3 C = outVertices[ic].pos;
            float area2 = glm::length(glm::cross(B - A, C - A));

            if (area2 < 1e-8f) {
                if (debug) std::cerr << "[degenerate] zero-area tri: "
                    << ia << "," << ib << "," << ic
                    << "  A=(" << A.x << "," << A.y << "," << A.z << ")"
                    << "  B=(" << B.x << "," << B.y << "," << B.z << ")"
                    << "  C=(" << C.x << "," << C.y << "," << C.z << ")\n";
                if (dropDegenerate) return;
            }

            outIndices.push_back(ia);
            outIndices.push_back(ib);
            outIndices.push_back(ic);
            };

        // Faces
        size_t idxCursor = 0;
        for (unsigned f = 0; f < m->face_count; ++f) {
            unsigned fv = m->face_vertices[f];
            if (debug) {
                std::cout << "Face " << f << " (fv=" << fv << "): ";
                for (unsigned k = 0; k < fv; ++k) {
                    const fastObjIndex ix = m->indices[idxCursor + k];
                    std::cout << "(" << ix.p << "/" << ix.t << "/" << ix.n << ") ";
                }
                std::cout << "\n";
            }

            std::vector<uint32_t> cornerIdx;
            cornerIdx.reserve(fv);
            for (unsigned k = 0; k < fv; ++k) {
                const fastObjIndex ix = m->indices[idxCursor + k];
                cornerIdx.push_back(emitVertex(ix));
            }

            // Fan triangulation
            for (unsigned i = 1; i + 1 < fv; ++i) {
                uint32_t i0 = cornerIdx[0];
                uint32_t i1 = cornerIdx[i];
                uint32_t i2 = cornerIdx[i + 1];
                if (flipWinding) std::swap(i1, i2);
                pushTri(i0, i1, i2);
            }

            idxCursor += fv;
        }

        fast_obj_destroy(m);

        if (debug) {
            std::cout << "Vertices: " << outVertices.size()
                << "  Indices: " << outIndices.size() << "\n";

            if (!outVertices.empty()) {
                std::cout << "Last vertices:\n";
                size_t start = outVertices.size() > 8 ? outVertices.size() - 8 : 0;
                for (size_t i = start; i < outVertices.size(); ++i) {
                    const auto& v = outVertices[i];
                    const auto& t = dbgTrip[i];
                    std::cout << "  VTX " << i
                        << "  p/t/n=" << t.p << "/" << t.t << "/" << t.n
                        << "  pos=(" << v.pos.x << "," << v.pos.y << "," << v.pos.z << ")"
                        << "  uv=(" << v.texCoord.x << "," << v.texCoord.y << ")"
                        << "  n=(" << v.normal.x << "," << v.normal.y << "," << v.normal.z << ")\n";
                }
            }

            if (!outIndices.empty()) {
                std::cout << "Last triangles (by indices):\n";
                uint32_t triCount = static_cast<uint32_t>(outIndices.size() / 3);
                uint32_t show = std::min<uint32_t>(6, triCount);
                for (uint32_t t = triCount - show; t < triCount; ++t) {
                    uint32_t ia = outIndices[3 * t + 0];
                    uint32_t ib = outIndices[3 * t + 1];
                    uint32_t ic = outIndices[3 * t + 2];
                    glm::vec3 A = outVertices[ia].pos;
                    glm::vec3 B = outVertices[ib].pos;
                    glm::vec3 C = outVertices[ic].pos;
                    std::cout << "  tri " << t << ": "
                        << ia << "," << ib << "," << ic
                        << "  A=(" << A.x << "," << A.y << "," << A.z << ")"
                        << "  B=(" << B.x << "," << B.y << "," << B.z << ")"
                        << "  C=(" << C.x << "," << C.y << "," << C.z << ")\n";
                }
            }
        }

        // ---- store in cache ----
        {
            std::lock_guard<std::mutex> lock(s_cacheMtx);
            s_cache.emplace(key, CacheEntry{ outVertices, outIndices });
        }

        return true;
    }

} // namespace ObjUtils
