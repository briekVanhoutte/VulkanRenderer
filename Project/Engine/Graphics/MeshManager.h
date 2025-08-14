// MeshManager.h
#pragma once
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <cstdint>

#include <Engine/Graphics/Mesh.h>
#include <Engine/Graphics/Material.h>
#include <Engine/Graphics/Vertex.h>  // your Vertex
#include <Engine/Core/Singleton.h>  // your Singleton<T>

class MeshManager : public Singleton<MeshManager> {
public:
    // Deduplicate by CPU geometry content (material is NOT part of the key).
    std::shared_ptr<Mesh> GetOrCreate(const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices,
        const std::shared_ptr<Material>& mat = {});

    // Canonical unit quad (for rectangles/planes)
    std::shared_ptr<Mesh> GetUnitQuad();

    void Clear();

private:
    friend class Singleton<MeshManager>;
    MeshManager() = default;

    // --- hashing helpers (FNV-1a 64) ---
    static uint64_t HashGeometry(const std::vector<Vertex>& v,
        const std::vector<uint32_t>& idx);
    static inline void Fnv1aAppend(uint64_t& h, const void* data, size_t nbytes);

    std::mutex mtx_;
    // Cache geometry by hash; weak_ptr lets meshes free when unused
    std::unordered_map<uint64_t, std::weak_ptr<Mesh>> cache_;
    std::weak_ptr<Mesh> unitQuad_;
};
