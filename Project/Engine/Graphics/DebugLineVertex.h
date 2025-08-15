// DebugLineVertex.h
#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <array>

struct DebugLineVertex {
    glm::vec3 pos;
    uint32_t  rgba; // packed RGBA8

    static VkVertexInputBindingDescription binding(uint32_t binding = 0) {
        VkVertexInputBindingDescription b{};
        b.binding = binding; b.stride = sizeof(DebugLineVertex);
        b.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return b;
    }
    static std::array<VkVertexInputAttributeDescription, 2> attributes(uint32_t binding = 0) {
        std::array<VkVertexInputAttributeDescription, 2> a{};
        a[0].location = 0; a[0].binding = binding;
        a[0].format = VK_FORMAT_R32G32B32_SFLOAT; a[0].offset = offsetof(DebugLineVertex, pos);
        a[1].location = 1; a[1].binding = binding;
        a[1].format = VK_FORMAT_R8G8B8A8_UNORM;   a[1].offset = offsetof(DebugLineVertex, rgba);
        return a;
    }
};