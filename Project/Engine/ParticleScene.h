#pragma once

#include "vulkan\vulkan_core.h"
#include "vulkanbase\VulkanUtil.h"
#include <glm/glm.hpp>
#include "Mesh.h" 
#include "objects/BaseObject.h"
#include "Engine/Scene.h"

#include "ParticleGroup.h"

class ParticleScene : public Scene {
public:

    void addParticleGroup(/*physx::PxVec4* particleBuffer, int ParticleCount ,*/ std::vector<Particle> particles)
    {
        ParticleGroup* object = new ParticleGroup{ /*particleBuffer , ParticleCount ,*/ particles};

        m_ParticleGroups.push_back(object);
    }

    void drawScene(VkPipelineLayout& pipelineLayout, VkCommandBuffer& buffer) {
        for (auto& object : m_ParticleGroups) {
            object->draw(pipelineLayout, buffer);
        }
    }

    void deleteScene(VkDevice device) {
        for (auto& object : m_ParticleGroups) {
            object->destroyParticleGroup(device);
        }
    }

private:
    std::vector<ParticleGroup*> m_ParticleGroups{};
};

