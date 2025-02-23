#pragma once

#include <vector>
#include <variant>
#include "MeshScene.h"
#include "ParticleScene.h"

// Enumeration to track the kind of object.
enum class SceneModelType {
    Mesh,
    Particle
};

// A unified scene object wrapper that holds either a mesh object or a particle group.
struct SceneObject {
    SceneModelType type;
    // Using a std::variant to store either a BaseObject* (from MeshScene)
    // or a ParticleGroup* (from ParticleScene).
    std::variant<BaseObject*, ParticleGroup*> object;
};

class SceneModelManager {
public:
    // Get the singleton instance.
    static SceneModelManager& getInstance() {
        static SceneModelManager instance;
        return instance;
    }

    // Delete copy and assignment.
    SceneModelManager(const SceneModelManager&) = delete;
    SceneModelManager& operator=(const SceneModelManager&) = delete;

    // Initialize both underlying scenes.
    void initScenes(VkPhysicalDevice& physicalDevice, VkDevice& device,
        const VkCommandPool& commandPool, const VkQueue& graphicsQueue) {
        m_meshScene->initObject(physicalDevice, device, commandPool, graphicsQueue);
        // (If your particle scene needs any initialization, do it here.)
    }

    // --- Functions to add objects to the scenes and to our unified list ---

    // Add a mesh model.
    BaseObject* addMeshModel(const std::vector<Vertex>& vertices,
        const std::vector<uint16_t>& indices,
        glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles)
    {
        // Add the object to the MeshScene.
        // (Assume addModel returns an index and that you add a corresponding getter.)
        unsigned int index = m_meshScene->addModel(vertices, indices, position, scale, rotationAngles);
        BaseObject* obj = m_meshScene->getBaseObject(index); // << You must implement getBaseObject in MeshScene.
        // Add to unified list.
        m_sceneObjects.push_back({ SceneModelType::Mesh, obj });
        return obj;
    }

    // Add a mesh rectangle.
    BaseObject* addMeshRectangle(const glm::vec3& normal,
        const glm::vec3& color,
        float width, float height,
        glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles)
    {
        unsigned int index = m_meshScene->addRectangle(normal, color, width, height, position, scale, rotationAngles);
        BaseObject* obj = m_meshScene->getBaseObject(index); // << Implement getBaseObject.
        m_sceneObjects.push_back({ SceneModelType::Mesh, obj });
        return obj;
    }

    
    // Add a particle group 
    ParticleGroup* addParticleGroup(physx::PxVec4* particleBuffer, int ParticleCount, std::vector<Particle> particles, glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles)
    {
        m_particleScene->addParticleGroup(particleBuffer,ParticleCount,particles,position,scale,rotationAngles);
        ParticleGroup* obj = m_particleScene->getLastParticleGroup(); // << Implement getLastParticleGroup.
        m_sceneObjects.push_back({ SceneModelType::Particle, obj });
        return obj;
    }

    // --- Access and update transforms for unified objects ---

    // Returns the unified list.
    const std::vector<SceneObject>& getSceneObjects() const {
        return m_sceneObjects;
    }

    // Update the transform (position, scale, rotation) for an object in the unified list.
    // This function checks the type and calls setPosition on the underlying object.
    void updateObjectTransform(size_t index, glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles) {
        if (index >= m_sceneObjects.size()) {
            // Handle error (for example, throw or log)
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

    // Clean up both scenes.
    void destroy(VkDevice device) {
        m_meshScene->deleteScene(device);
        m_particleScene->deleteScene(device);
    }

    // (Optionally provide access to the underlying scenes if needed.)
    MeshScene* getMeshScene() { return m_meshScene; }
    ParticleScene* getParticleScene() { return m_particleScene; }

private:
    SceneModelManager() {
        m_meshScene = new MeshScene();
        m_particleScene = new ParticleScene();
    }

    // Destructor cleans up the allocated objects.
    ~SceneModelManager() {
        delete m_meshScene;
        delete m_particleScene;
    }

    MeshScene* m_meshScene;
    ParticleScene* m_particleScene;
    // A unified list of pointers to every object added.
    std::vector<SceneObject> m_sceneObjects;
};
