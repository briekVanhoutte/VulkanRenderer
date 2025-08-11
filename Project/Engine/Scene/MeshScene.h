#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <vulkan\vulkan_core.h>
#include <glm/glm.hpp>
#include <Engine/Graphics/Mesh.h>
#include <Engine/Scene/GameObjects/BaseObject.h>
#include <Engine/Scene/Scene.h>
#include <glm\gtx\quaternion.hpp>

static inline void basisFromNormal(const glm::vec3& nIn,
    glm::vec3& t, glm::vec3& b, glm::vec3& n)
{
    n = glm::normalize(nIn);
    // Choose an up reference that isn't parallel to n
    glm::vec3 up = (std::abs(n.y) > 0.999f) ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
    t = glm::normalize(glm::cross(up, n)); // tangent (→ width axis)
    b = glm::cross(n, t);                  // bitangent (→ height axis)
}

enum class ObjType {
    plane,
    model
};


class MeshScene : public Scene{
public:

    unsigned int addModel(const std::vector<Vertex>& Vertexes, const std::vector<uint16_t>& indices, glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles, const std::shared_ptr<Material> mat = {})
    {
        BaseObject* object = new BaseObject{ Vertexes, indices ,mat };

        object->setPosition(position, scale, rotationAngles);
        m_BaseObjects.push_back(object);

        return m_BaseObjects.size() - 1;
    }

    BaseObject* getBaseObject(unsigned int modelId)
    {
        if (modelId < m_BaseObjects.size())
        {
            return m_BaseObjects[modelId];
        }
        return nullptr;
    }

    unsigned int addRectangle(const glm::vec3& normal,
        const glm::vec3& color,
        float width, float height,
        glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles,
        const std::shared_ptr<Material> mat = {})
    {
        float halfW = width * 0.5f;
        float halfH = height * 0.5f;

        // Build a stable local frame from the normal
        glm::vec3 T, B, N;
        basisFromNormal(normal, T, B, N);

        // Positions in that frame: X→T (width), Y→B (height), Z along N
        glm::vec3 p0 = (-halfW) * T + (-halfH) * B; // bottom-left
        glm::vec3 p1 = (+halfW) * T + (-halfH) * B; // bottom-right
        glm::vec3 p2 = (+halfW) * T + (+halfH) * B; // top-right
        glm::vec3 p3 = (-halfW) * T + (+halfH) * B; // top-left

        glm::vec2 uv0(0.0f, 0.0f);
        glm::vec2 uv1(1.0f, 0.0f);
        glm::vec2 uv2(1.0f, 1.0f);
        glm::vec2 uv3(0.0f, 1.0f);

        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;

        vertices.push_back({ p0, N, color, uv0 });
        vertices.push_back({ p1, N, color, uv1 });
        vertices.push_back({ p2, N, color, uv2 });
        vertices.push_back({ p3, N, color, uv3 });

        uint16_t base = 0;
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);

        BaseObject* object = new BaseObject{ vertices, indices, mat };
        object->setPosition(position, scale, rotationAngles); // world transform
        m_BaseObjects.push_back(object);
        return static_cast<unsigned int>(m_BaseObjects.size() - 1);
    }

    void initObject(VkPhysicalDevice& physicalDevice, VkDevice& device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue) {
        for (auto& object : m_BaseObjects) {
            object->init(physicalDevice, device, commandPool, graphicsQueue);
        }
    }

    void drawScene(VkPipelineLayout& pipelineLayout, VkCommandBuffer& buffer) {
        for (auto& object : m_BaseObjects) {
            object->draw(pipelineLayout, buffer);
        }
    }

    void updateLocationObject(unsigned int pos, glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles) {
        if (pos < m_BaseObjects.size()) {
            m_BaseObjects[pos]->setPosition(position, scale, rotationAngles);
        }
    }

    glm::vec3 getLocation(unsigned int pos) {
        if (pos < m_BaseObjects.size()) {
            return m_BaseObjects[pos]->getPosition();
        }
    }

    void deleteScene(VkDevice device) {
        for (auto& object : m_BaseObjects) {
            object->destroy(device);
        }
    }

private:
    std::vector<BaseObject*> m_BaseObjects{};
};

