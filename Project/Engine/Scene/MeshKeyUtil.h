#pragma once
#include <vulkan/vulkan_core.h>
#include <cstdint>
#include <functional>

// Forward declare to avoid heavy includes first
class BaseObject;

struct MeshKey {
    VkBuffer vertexBuffer{};
    VkDeviceSize vbOffset{};
    VkBuffer indexBuffer{};
    VkDeviceSize ibOffset{};
    uint32_t indexCount{};
    uint32_t pipelineIndex{};
    uint64_t logicalId{ 0 }; // keep only for debug printing

    bool operator==(const MeshKey& o) const {
        return vertexBuffer == o.vertexBuffer &&
            vbOffset == o.vbOffset &&
            indexBuffer == o.indexBuffer &&
            ibOffset == o.ibOffset &&
            indexCount == o.indexCount &&
            pipelineIndex == o.pipelineIndex;
        // NOTE: logicalId intentionally ignored -> also ignore in hash
    }
};

struct MeshKeyHash {
    size_t operator()(const MeshKey& k) const noexcept {
        size_t h = std::hash<uint64_t>{}(reinterpret_cast<uint64_t>(k.vertexBuffer));
        h ^= std::hash<uint64_t>{}(reinterpret_cast<uint64_t>(k.indexBuffer)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<uint64_t>{}(k.vbOffset) + (h << 6) + (h >> 2);
        h ^= std::hash<uint64_t>{}(k.ibOffset) + (h << 6) + (h >> 2);
        h ^= std::hash<uint32_t>{}(k.indexCount) + (h << 6) + (h >> 2);
        h ^= std::hash<uint32_t>{}(k.pipelineIndex);
        // DO NOT hash logicalId if it's not in operator==.
        return h;
    }
};


#include <Engine/Scene/GameObjects/BaseObject.h>

inline MeshKey MakeMeshKey(const BaseObject* obj, uint32_t pipelineIndex) {
    return MeshKey{
        obj->getVertexBuffer(),
        obj->getVBOffset(),
        obj->getIndexBuffer(),
        obj->getIBOffset(),
        obj->getIndexCount(),
        //obj->getMaterial().get(),
        pipelineIndex,
        obj->getLogicalGroupId()   // << NEW
    };
}