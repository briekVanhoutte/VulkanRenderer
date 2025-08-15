#pragma once
#include <glm/glm.hpp>

struct MeshData {
    glm::mat4 model;
    uint32_t AlbedoID;
    uint32_t NormalMapID;
    uint32_t MetalnessID;
    uint32_t RoughnessID;
    uint32_t HeightMapID;
};

struct DebugLinePC {
    glm::mat4 world;   // 64 bytes
    float     lineWidth; // 4 bytes (optional, still call vkCmdSetLineWidth)
    float     _pad[3];   // pad to 16-byte boundary (total 80 bytes)
};