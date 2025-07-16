#pragma once

#include <glm/glm.hpp>
#include <Engine/Scene/Mesh.h>
#include <memory>

class BaseObject {
public:
    BaseObject(const std::vector<Vertex>& Vertexes, const std::vector<uint16_t>& indices) {
        mesh = std::make_unique<Mesh>(Vertexes, indices);
    }
   
    void draw(VkPipelineLayout& pipelineLayout, VkCommandBuffer& buffer) {
        mesh->draw(pipelineLayout, buffer);
    }

    void setPosition(glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles) {
        mesh->setPosition(position, scale, rotationAngles);
    }

    glm::vec3 getPosition() {
        return mesh->getPostion();
    }

    void init(VkPhysicalDevice& physicalDevice, VkDevice& device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue) {
        mesh->initialize(physicalDevice, device, commandPool, graphicsQueue);
    }

    void destroy(VkDevice device) {
        mesh->destroyMesh(device);
    }

protected:
    std::unique_ptr<Mesh> mesh;
};