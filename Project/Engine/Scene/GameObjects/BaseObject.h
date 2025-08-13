#pragma once

#include <glm/glm.hpp>
#include <Engine/Graphics/Mesh.h>
#include <memory>

class BaseObject {
public:
    BaseObject(const std::vector<Vertex>& Vertexes, const std::vector<uint32_t>& indices, const std::shared_ptr<Material> mat = {}) {
        mesh = std::make_unique<Mesh>(Vertexes, indices, mat);
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

    // --- NEW: pass-through getters for batching ---
    VkBuffer getVertexBuffer() const { return mesh->getVertexBuffer(); }
    VkBuffer getIndexBuffer()  const { return mesh->getIndexBuffer(); }
    VkDeviceSize getVBOffset() const { return mesh->getVBOffset(); }
    VkDeviceSize getIBOffset() const { return mesh->getIBOffset(); }
    uint32_t getIndexCount()   const { return mesh->getIndexCount(); }
    const std::shared_ptr<Material>& getMaterial() const { return mesh->getMaterial(); }
    bool isInitialized() const { return mesh->isInitialized(); }

    // Optional:
    Mesh* rawMesh() { return mesh.get(); }
    const Mesh* rawMesh() const { return mesh.get(); }

    void setLogicalGroupId(uint64_t id) { m_logicalGroupId = id; }
    uint64_t getLogicalGroupId() const { return m_logicalGroupId; }

protected:
    std::unique_ptr<Mesh> mesh;
    uint64_t m_logicalGroupId = 0;   // 0 = not grouped
};