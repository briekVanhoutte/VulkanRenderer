#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <vulkan\vulkan_core.h>
#include <glm/glm.hpp>
#include <Engine/Graphics/Mesh.h>
#include <Engine/Scene/GameObjects/BaseObject.h>
#include <Engine/Scene/Scene.h>
#include <glm\gtx\quaternion.hpp>
#include "MeshKeyUtil.h"
#include "ChunkGrid.h"
#include <unordered_map>
#include <Engine/Graphics/InstanceData.h>
#include <Engine/Graphics/VulkanVars.h>
#include <Engine/Graphics/MeshManager.h>

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
    explicit MeshScene(float chunkSize = 32.f) : m_chunks(chunkSize) {}


    void setObjectCoverageOverride(BaseObject* obj, glm::vec3 center, glm::vec3 halfExtents) {
        m_chunks.setCoverageOverride(obj, center, halfExtents);
    }
    void clearObjectCoverageOverride(BaseObject* obj) {
        m_chunks.clearCoverageOverride(obj);
    }

    unsigned int addModel(const std::vector<Vertex>& Vertexes, const std::vector<uint32_t>& indices, glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles, const std::shared_ptr<Material> mat = {})
    {
        BaseObject* object = new BaseObject{ Vertexes, indices, mat };
        object->setPosition(position, scale, rotationAngles);
        m_BaseObjects.push_back(object);
        m_pendingToRegister.push_back(object);
        return static_cast<unsigned int>(m_BaseObjects.size() - 1);
    }

    void setObjectGlobal(BaseObject* obj, bool enable) { m_chunks.setGlobal(obj, enable); }
    void setObjectMultiChunk(BaseObject* obj, glm::vec3 halfExtents) { m_chunks.setMultiChunk(obj, halfExtents); }

    void setObjectGlobal(unsigned int idx, bool enable) {
        if (idx < m_BaseObjects.size()) m_chunks.setGlobal(m_BaseObjects[idx], enable);
    }
    void setObjectMultiChunk(unsigned int idx, glm::vec3 halfExtents) {
        if (idx < m_BaseObjects.size()) m_chunks.setMultiChunk(m_BaseObjects[idx], halfExtents);
    }

    BaseObject* getBaseObject(unsigned int modelId)
    {
        if (modelId < m_BaseObjects.size())
        {
            return m_BaseObjects[modelId];
        }
        return nullptr;
    }

    const ChunkGrid& getChunkGrid() const { return m_chunks; }
    ChunkGrid& getChunkGrid() { return m_chunks; }

    unsigned int addRectangle(const glm::vec3& normal,
        const glm::vec3& color,
        float width, float height,
        glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles,
        const std::shared_ptr<Material> mat = {})
    {
        auto quad = MeshManager::GetInstance().GetUnitQuad();
        auto* object = new BaseObject{ quad, mat };

        // align +Z to 'normal'
        glm::vec3 n = glm::normalize(normal);
        glm::quat qAlign = glm::rotation(glm::vec3(0, 0, 1), n);
        glm::quat qUser = glm::quat(glm::radians(rotationAngles));
        glm::quat qFinal = qUser * qAlign;

        glm::vec3 finalScale = scale * glm::vec3(width, height, 1.0f);
        object->setPosition(position, finalScale, glm::degrees(glm::eulerAngles(qFinal)));

        m_BaseObjects.push_back(object);
        m_pendingToRegister.push_back(object);
        return static_cast<unsigned int>(m_BaseObjects.size() - 1);
    }

    void initObject(VkPhysicalDevice& physicalDevice, VkDevice& device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue) {
        for (auto& object : m_BaseObjects) {
            object->init(physicalDevice, device, commandPool, graphicsQueue);
        }
        // Now buffers exist → register into chunks with MeshKey (pipelineIndex=0 for 3D)
        for (BaseObject* o : m_pendingToRegister) {
            if (o->isInitialized())
                m_chunks.add(o, MakeMeshKey(o, 0));
        }
        m_pendingToRegister.clear();
    }

    void setFrameView(const glm::vec3& camPos, float renderDistance,
        const glm::vec3& camForward,
        bool use2D,
        float frontConeDegrees,
        bool useCenterTest) 
    {
        m_chunks.setCulling(
            camPos,
            renderDistance,
            camForward,   // your view dir
            use2D,            // use2D (ignore Y)
            frontConeDegrees,          // front-cone degrees; -1 disables
            useCenterTest             // use center-distance test
        );;
    }

    void updateLocationObject(unsigned int pos, glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles) {
        if (pos >= m_BaseObjects.size()) return;
        BaseObject* obj = m_BaseObjects[pos];
        obj->setPosition(position, scale, rotationAngles);
        m_chunks.update(obj, MakeMeshKey(obj, 0));
    }

    void notifyMoved(BaseObject* obj) {
        if (!obj) return;
        m_chunks.update(obj, MakeMeshKey(obj, 0));
    }

    glm::vec3 getLocation(unsigned int pos) {
        if (pos < m_BaseObjects.size()) return m_BaseObjects[pos]->getPosition();
        return glm::vec3(0);
    }

    void deleteScene(VkDevice device) override {
        for (auto& object : m_BaseObjects) {
            m_chunks.remove(object);
            object->destroy(device);
            delete object;
        }
        m_BaseObjects.clear();
    }

    void drawScene(VkPipelineLayout& pipelineLayout, VkCommandBuffer& cmd);
    void debugPrintVisibleBatches(std::ostream& os);

    void setChunksEnabled(bool enabled) { m_chunksEnabled = enabled; }
    bool chunksEnabled() const { return m_chunksEnabled; }

private:
    std::vector<BaseObject*> m_BaseObjects{};
    std::vector<BaseObject*> m_pendingToRegister{};
    std::unordered_map<MeshKey, std::array<std::unique_ptr<DataBuffer>, MAX_FRAMES_IN_FLIGHT>, MeshKeyHash> m_instanceBuffers;
    DataBuffer* getOrGrowInstanceBuffer(const MeshKey& key, size_t neededCount);
    
    ChunkGrid m_chunks;
    bool m_chunksEnabled = true;
};

