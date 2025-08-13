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
        std::vector<uint32_t> indices;

        vertices.push_back({ p0, N, color, uv0 });
        vertices.push_back({ p1, N, color, uv1 });
        vertices.push_back({ p2, N, color, uv2 });
        vertices.push_back({ p3, N, color, uv3 });

        uint32_t base = 0;
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);

        BaseObject* object = new BaseObject{ vertices, indices, mat };
        object->setPosition(position, scale, rotationAngles);
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

    void setFrameView(const glm::vec3& cameraPos, float renderDistance) {
        m_chunks.setCulling(cameraPos, renderDistance);
    }

    void drawScene(VkPipelineLayout& pipelineLayout, VkCommandBuffer& buffer) override {
        m_chunks.forVisibleBatches([&](const MeshKey& key, const std::vector<BaseObject*>& batch) {
            for (BaseObject* obj : batch) {
                obj->draw(pipelineLayout, buffer);
            }
            });
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

private:
    std::vector<BaseObject*> m_BaseObjects{};
    std::vector<BaseObject*> m_pendingToRegister{};
    ChunkGrid m_chunks;
};

