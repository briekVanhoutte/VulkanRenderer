// MeshManager.cpp
#include "MeshManager.h"

static constexpr uint64_t FNV1A64_OFFSET = 1469598103934665603ull;
static constexpr uint64_t FNV1A64_PRIME = 1099511628211ull;

void MeshManager::Fnv1aAppend(uint64_t& h, const void* data, size_t nbytes) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < nbytes; ++i) { h ^= p[i]; h *= FNV1A64_PRIME; }
}

uint64_t MeshManager::HashGeometry(const std::vector<Vertex>& v,
    const std::vector<uint32_t>& idx) {
    uint64_t h = FNV1A64_OFFSET;
    for (const Vertex& vert : v) {
        Fnv1aAppend(h, &vert.pos, sizeof(vert.pos));
        Fnv1aAppend(h, &vert.normal, sizeof(vert.normal));
        Fnv1aAppend(h, &vert.color, sizeof(vert.color));
        Fnv1aAppend(h, &vert.texCoord, sizeof(vert.texCoord));
    }
    if (!idx.empty()) Fnv1aAppend(h, idx.data(), idx.size() * sizeof(uint32_t));
    return h;
}

std::shared_ptr<Mesh> MeshManager::GetOrCreate(const std::vector<Vertex>& vertices,
    const std::vector<uint32_t>& indices,
    const std::shared_ptr<Material>& mat)
{
    const uint64_t key = HashGeometry(vertices, indices);
    std::lock_guard<std::mutex> lock(mtx_);

    if (auto existing = cache_[key].lock()) {
        if (mat) existing->setMaterial(mat);  // optional: keep legacy path happy
        return existing;
    }

    auto mesh = std::make_shared<Mesh>(vertices, indices, mat);
    cache_[key] = mesh;
    return mesh;
}

std::shared_ptr<Mesh> MeshManager::GetUnitQuad() {
    if (auto q = unitQuad_.lock()) return q;

    std::vector<Vertex> v = {
        {{-0.5f,-0.5f,0.f},{0,0,1},{1,1,1},{0,0}},
        {{ 0.5f,-0.5f,0.f},{0,0,1},{1,1,1},{1,0}},
        {{ 0.5f, 0.5f,0.f},{0,0,1},{1,1,1},{1,1}},
        {{-0.5f, 0.5f,0.f},{0,0,1},{1,1,1},{0,1}},
    };
    std::vector<uint32_t> idx = { 0,1,2, 0,2,3 };

    auto quad = std::make_shared<Mesh>(v, idx, nullptr);
    unitQuad_ = quad;
    return quad;
}

void MeshManager::Clear() {
    std::lock_guard<std::mutex> lock(mtx_);
    cache_.clear();
    unitQuad_.reset();
}
