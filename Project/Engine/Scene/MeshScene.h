#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <vulkan\vulkan_core.h>
#include <glm/glm.hpp>
#include <Engine/Scene/Mesh.h>
#include <Engine/Scene/GameObjects/BaseObject.h>
#include <Engine/Scene/Scene.h>
#include <glm\gtx\quaternion.hpp>


enum class ObjType {
    plane,
    model
};


class MeshScene : public Scene{
public:

    unsigned int addModel(const std::vector<Vertex>& Vertexes, const std::vector<uint16_t>& indices, glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles)
    {
        BaseObject* object = new BaseObject{ Vertexes, indices };

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

    unsigned int addRectangle( const glm::vec3& normal,
        const glm::vec3& color,
        float width,
        float height, glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles)
    {
        // Calculate half dimensions
        float halfWidth = width / 2.0f;
        float halfHeight = height / 2.0f;

        // Define the four corners of the rectangle
        glm::vec3 p0(-halfWidth, -halfHeight, 0.0f);
        glm::vec3 p1(halfWidth, -halfHeight, 0.0f);
        glm::vec3 p2(halfWidth, halfHeight, 0.0f);
        glm::vec3 p3(-halfWidth, halfHeight, 0.0f);

        std::vector<Vertex> vertices{};
        std::vector<uint16_t> indices{};
        // Default normal (pointing in the Z direction)
        glm::vec3 defaultNormal(0.0f, 0.0f, 1.0f);

        // Calculate rotation quaternion to align default normal with the given normal
        glm::quat rotationQuat = glm::rotation(defaultNormal, normal);

        // Rotate the vertices
        p0 = rotationQuat * p0;
        p1 = rotationQuat * p1;
        p2 = rotationQuat * p2;
        p3 = rotationQuat * p3;

        // Define vertices for the rectangle with the correct normal and color
        vertices.push_back({ p0, normal, color });
        vertices.push_back({ p1, normal, color });
        vertices.push_back({ p2, normal, color });
        vertices.push_back({ p3, normal, color });

        // Define indices for the two triangles
        uint16_t baseIndex = static_cast<uint16_t>(vertices.size()) - 4;
        indices.push_back(baseIndex + 0);
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);

        indices.push_back(baseIndex + 0);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 3);


        BaseObject* object = new BaseObject{ vertices, indices };

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

