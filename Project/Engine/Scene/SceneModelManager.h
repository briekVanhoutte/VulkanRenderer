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
        const std::vector<uint16_t>& indices,
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

    void updateObjectTransform(size_t index, glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles) {
        if (index >= m_sceneObjects.size()) {
            return;
        }
        const SceneObject& sceneObj = m_sceneObjects[index];
        if (sceneObj.type == SceneModelType::Mesh) {
            BaseObject* obj = std::get<BaseObject*>(sceneObj.object);
            if (obj)
                obj->setPosition(position, scale, rotationAngles);
        }
        else if (sceneObj.type == SceneModelType::Particle) {
            ParticleGroup* obj = std::get<ParticleGroup*>(sceneObj.object);
            if (obj)
                obj->setPosition(position, scale, rotationAngles);
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
