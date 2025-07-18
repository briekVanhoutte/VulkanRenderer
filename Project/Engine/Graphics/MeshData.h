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