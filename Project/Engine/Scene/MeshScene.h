#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <vulkan\vulkan_core.h>
#include <glm/glm.hpp>
#include <Engine/Graphics/Mesh.h>
#include <Engine/Scene/GameObjects/BaseObject.h>
#include <Engine/Scene/Scene.h>
#include <glm\gtx\quaternion.hpp>


enum class ObjType {
    plane,
    model
};


class MeshScene : public Scene{
public:

    unsigned int addModel(const std::vector<Vertex>& Vertexes, const std::vector<uint16_t>& indices, glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles, const std::string& TexturePath = {})
    {
        BaseObject* object = new BaseObject{ Vertexes, indices ,TexturePath };

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
        float width,
        float height, glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles, const std::string& TexturePath = {})
    {
        float halfWidth = width / 2.0f;
        float halfHeight = height / 2.0f;

        glm::vec3 p0(-halfWidth, -halfHeight, 0.0f); // bottom left
        glm::vec3 p1(halfWidth, -halfHeight, 0.0f);  // bottom right
        glm::vec3 p2(halfWidth, halfHeight, 0.0f);   // top right
        glm::vec3 p3(-halfWidth, halfHeight, 0.0f);  // top left

        // UVs: (0,0)=bottom left, (1,0)=bottom right, (1,1)=top right, (0,1)=top left
        glm::vec2 uv0(0.0f, 0.0f); // bottom left
        glm::vec2 uv1(1.0f, 0.0f); // bottom right
        glm::vec2 uv2(1.0f, 1.0f); // top right
        glm::vec2 uv3(0.0f, 1.0f); // top left

        std::vector<Vertex> vertices{};
        std::vector<uint16_t> indices{};
        glm::vec3 defaultNormal(0.0f, 0.0f, 1.0f);

        glm::quat rotationQuat = glm::rotation(defaultNormal, normal);

        p0 = rotationQuat * p0;
        p1 = rotationQuat * p1;
        p2 = rotationQuat * p2;
        p3 = rotationQuat * p3;

        // Make sure your Vertex struct is Vertex(glm::vec3 pos, glm::vec3 normal, glm::vec3 color, glm::vec2 texCoord)
        vertices.push_back({ p0, normal, color, uv0 }); // bottom left
        vertices.push_back({ p1, normal, color, uv1 }); // bottom right
        vertices.push_back({ p2, normal, color, uv2 }); // top right
        vertices.push_back({ p3, normal, color, uv3 }); // top left

        uint16_t baseIndex = static_cast<uint16_t>(vertices.size()) - 4;
        indices.push_back(baseIndex + 0); // bottom left
        indices.push_back(baseIndex + 1); // bottom right
        indices.push_back(baseIndex + 2); // top right

        indices.push_back(baseIndex + 0); // bottom left
        indices.push_back(baseIndex + 2); // top right
        indices.push_back(baseIndex + 3); // top left

        BaseObject* object = new BaseObject{ vertices, indices,TexturePath};
        object->setPosition(position, scale, rotationAngles);

        m_BaseObjects.push_back(object);
        return m_BaseObjects.size() - 1;
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

