#pragma once

#include "vulkan\vulkan_core.h"
#include "vulkanbase\VulkanUtil.h"
#include <glm/glm.hpp>
#include "Mesh.h" 
#include "objects/BaseObject.h"

enum ObjectType
{
    Rect,
    Ov,
    RounRect
};

class Scene {
public:

    void addModel(const std::vector<Vertex>& Vertexes, const std::vector<uint16_t>& indices, glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles)
    {
        BaseObject* object = new BaseObject{ Vertexes, indices };

        object->setPosition(position,scale,rotationAngles);
        m_BaseObjects.push_back(object);
    }

    void initObject(VkPhysicalDevice& physicalDevice, VkDevice& device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue) {
        for (auto& object : m_BaseObjects) {
            object->init( physicalDevice,  device, commandPool, graphicsQueue);
        }
    }

    void drawScene(VkPipelineLayout& pipelineLayout, VkCommandBuffer& buffer) {
        for (auto& object : m_BaseObjects) {
            object->draw(pipelineLayout, buffer);
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

