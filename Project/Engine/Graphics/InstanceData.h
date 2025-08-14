// Engine/Graphics/InstanceData.h
#pragma once
#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>
#include <vector>

struct InstanceData {
    glm::mat4 model;     // 64
    glm::uvec4 texIds0;  // 16
    uint32_t   heightId; // 4
    uint32_t   pad[3]{};

    static VkVertexInputBindingDescription getBindingDescription(uint32_t binding = 1) {
        VkVertexInputBindingDescription d{};
        d.binding = binding;
        d.stride = sizeof(InstanceData);
        d.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
        return d;
    }

    static std::vector<VkVertexInputAttributeDescription>
        getAttributeDescriptions(uint32_t binding = 1, uint32_t startLocation = 4) {
        std::vector<VkVertexInputAttributeDescription> a(6);

        const uint32_t base = static_cast<uint32_t>(offsetof(InstanceData, model));
        for (int i = 0; i < 4; ++i) {
            a[i].binding = binding;
            a[i].location = startLocation + i;               // 4,5,6,7
            a[i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            a[i].offset = base + sizeof(glm::vec4) * i;    // <-- FIX
        }

        a[4].binding = binding;
        a[4].location = startLocation + 4;                   // 8
        a[4].format = VK_FORMAT_R32G32B32A32_UINT;
        a[4].offset = static_cast<uint32_t>(offsetof(InstanceData, texIds0));

        a[5].binding = binding;
        a[5].location = startLocation + 5;                   // 9
        a[5].format = VK_FORMAT_R32_UINT;
        a[5].offset = static_cast<uint32_t>(offsetof(InstanceData, heightId));
        return a;
    }
};
