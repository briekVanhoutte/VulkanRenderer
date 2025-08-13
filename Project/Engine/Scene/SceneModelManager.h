#pragma once

#include <vector>
#include <variant>
#include "MeshScene.h"
#include "ParticleScene.h"

enum class SceneModelType {
    Mesh,
    Particle
};

struct SceneObject {
    SceneModelType type;
    std::variant<BaseObject*, ParticleGroup*> object;
};

class SceneModelManager {
public:
    static SceneModelManager& getInstance() {
        static SceneModelManager instance;
        return instance;
    }

    SceneModelManager(const SceneModelManager&) = delete;
    SceneModelManager& operator=(const SceneModelManager&) = delete;

    void initScenes(VkPhysicalDevice& physicalDevice, VkDevice& device,
        const VkCommandPool& commandPool, const VkQueue& graphicsQueue) {
        m_meshScene->initObject(physicalDevice, device, commandPool, graphicsQueue);
       
    }

    BaseObject* addMeshModel(const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices,
        glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles, const std::shared_ptr<Material> mat = {})
    {
        unsigned int index = m_meshScene->addModel(vertices, indices, position, scale, rotationAngles, mat);
        BaseObject* obj = m_meshScene->getBaseObject(index); 
        m_sceneObjects.push_back({ SceneModelType::Mesh, obj });
        return obj;
    }

    BaseObject* addMeshRectangle(const glm::vec3& normal,
        const glm::vec3& color,
        float width, float height,
        glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles, const std::shared_ptr<Material> mat = {})
    {
        unsigned int index = m_meshScene->addRectangle(normal, color, width, height, position, scale, rotationAngles, mat);
        BaseObject* obj = m_meshScene->getBaseObject(index); 
        m_sceneObjects.push_back({ SceneModelType::Mesh, obj });
        return obj;
    }

    ParticleGroup* addParticleGroup(physx::PxVec4* particleBuffer, int ParticleCount, std::vector<Particle> particles, glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles)
    {
        m_particleScene->addParticleGroup(particleBuffer,ParticleCount,particles,position,scale,rotationAngles);
        ParticleGroup* obj = m_particleScene->getLastParticleGroup(); 
        m_sceneObjects.push_back({ SceneModelType::Particle, obj });
        return obj;
    }

    const std::vector<SceneObject>& getSceneObjects() const {
        return m_sceneObjects;
    }

    void setFrameView(const glm::vec3& cameraPos, float renderDistance) {
        if (m_meshScene) m_meshScene->setFrameView(cameraPos, renderDistance);
        // ParticleScene can stay as-is for now.
    }

    void updateObjectTransform(size_t index, glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles) {
        if (index >= m_sceneObjects.size()) return;
        const SceneObject& so = m_sceneObjects[index];
        if (so.type == SceneModelType::Mesh) {
            auto* obj = std::get<BaseObject*>(so.object);
            if (!obj) return;
            obj->setPosition(position, scale, rotationAngles);
            // NEW: make chunk membership consistent
            if (m_meshScene) m_meshScene->notifyMoved(obj);
        }
        else if (so.type == SceneModelType::Particle) {
            auto* obj = std::get<ParticleGroup*>(so.object);
            if (obj) obj->setPosition(position, scale, rotationAngles);
        }
    }

    void destroy(VkDevice device) {
        m_meshScene->deleteScene(device);
        m_particleScene->deleteScene(device);
    }

    MeshScene* getMeshScene() { return m_meshScene; }
    ParticleScene* getParticleScene() { return m_particleScene; }

private:
    SceneModelManager() {
        m_meshScene = new MeshScene();
        m_particleScene = new ParticleScene();
    }

    ~SceneModelManager() {
        delete m_meshScene;
        delete m_particleScene;
    }

    MeshScene* m_meshScene;
    ParticleScene* m_particleScene;
    std::vector<SceneObject> m_sceneObjects;
};
