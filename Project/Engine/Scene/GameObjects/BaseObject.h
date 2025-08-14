// BaseObject.h
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>

#include <Engine/Graphics/Mesh.h>
#include <Engine/Graphics/Material.h>
#include <Engine/Graphics/MeshManager.h>

class BaseObject {
public:
    // SAME signature as before (call sites unchanged)
    BaseObject(const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices,
        const std::shared_ptr<Material> mat = {})
        : mesh(MeshManager::GetInstance().GetOrCreate(vertices, indices, mat))
        , m_material(mat)
    {
        recalcModel();
    }

    // Also allow direct shared mesh
    explicit BaseObject(std::shared_ptr<Mesh> sharedMesh,
        const std::shared_ptr<Material>& mat = {})
        : mesh(std::move(sharedMesh))
        , m_material(mat)
    {
        if (mesh && mat) mesh->setMaterial(mat); // legacy-only
        recalcModel();
    }

    void draw(VkPipelineLayout& pipelineLayout, VkCommandBuffer& buffer) {
        // Push per-object constants (model + tex IDs) before drawing the shared mesh
        struct Push {
            glm::mat4 model;
            uint32_t AlbedoID, NormalMapID, MetalnessID, RoughnessID, HeightMapID;
        } pc{};

        pc.model = m_model;
        auto& m = getMaterial();
        pc.AlbedoID = m ? m->getAlbedoMapID() : 0u;
        pc.NormalMapID = m ? m->getNormalMapID() : 0u;
        pc.MetalnessID = m ? m->getMetalnessMapID() : 0u;
        pc.RoughnessID = m ? m->getRoughnessMapID() : 0u;
        pc.HeightMapID = m ? m->getHeightMapID() : 0u;

        vkCmdPushConstants(buffer, pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(Push), &pc);

        mesh->draw(pipelineLayout, buffer);
    }

    // Keep the public API the same, but manage transform per-object
    void setPosition(glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAnglesDeg) {
        m_pos = position; m_scale = scale; m_rotDeg = rotationAnglesDeg;
        recalcModel();
    }

    glm::vec3 getPosition() { return m_pos; }

    glm::mat4 getModelMatrix() const { return m_model; }

    void init(VkPhysicalDevice& physicalDevice, VkDevice& device,
        const VkCommandPool& commandPool, const VkQueue& graphicsQueue) {
        if (mesh) mesh->initialize(physicalDevice, device, commandPool, graphicsQueue);
    }

    void destroy(VkDevice device) { if (mesh) mesh->destroyMesh(device); }

    // --- pass-through for batching ---
    VkBuffer getVertexBuffer() const { return mesh->getVertexBuffer(); }
    VkBuffer getIndexBuffer()  const { return mesh->getIndexBuffer(); }
    VkDeviceSize getVBOffset() const { return mesh->getVBOffset(); }
    VkDeviceSize getIBOffset() const { return mesh->getIBOffset(); }
    uint32_t getIndexCount()   const { return mesh->getIndexCount(); }

    // Per-object material (falls back to mesh default if unset)
    const std::shared_ptr<Material>& getMaterial() const {
        return m_material ? m_material : mesh->getMaterial();
    }
    void setMaterial(std::shared_ptr<Material> m) { m_material = std::move(m); }

    bool isInitialized() const { return mesh->isInitialized(); }

    void setLogicalGroupId(uint64_t id) { m_logicalGroupId = id; }
    uint64_t getLogicalGroupId() const { return m_logicalGroupId; }

    Mesh* rawMesh() { return mesh.get(); }
    const Mesh* rawMesh() const { return mesh.get(); }

private:
    void recalcModel() {
        // ZYX euler (deg) → radians
        glm::vec3 r = glm::radians(m_rotDeg);
        glm::mat4 T = glm::translate(glm::mat4(1), m_pos);
        glm::mat4 R =
            glm::rotate(glm::mat4(1), r.z, { 0,0,1 }) *
            glm::rotate(glm::mat4(1), r.y, { 0,1,0 }) *
            glm::rotate(glm::mat4(1), r.x, { 1,0,0 });
        glm::mat4 S = glm::scale(glm::mat4(1), m_scale);
        m_model = T * R * S;
    }

    // Shared geometry
    std::shared_ptr<Mesh> mesh;

    // Per-object data
    std::shared_ptr<Material> m_material;
    glm::vec3 m_pos{ 0 };
    glm::vec3 m_scale{ 1 };
    glm::vec3 m_rotDeg{ 0 };
    glm::mat4 m_model{ 1 };

    uint64_t m_logicalGroupId = 0;
};
