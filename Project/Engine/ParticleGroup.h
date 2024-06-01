#pragma once

#include "vulkan\vulkan_core.h"
#include "vulkanbase\VulkanUtil.h"
#include <glm/glm.hpp>
#include "Engine\DataBuffer.h"
//#include "PxParticleBuffer.h"

class ParticleGroup {
public:
	ParticleGroup(/*physx::PxVec4* particleBuffer, int ParticleCount,*/ std::vector<Particle> particles);
	void initialize(VkPhysicalDevice physicalDevice, VkDevice device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue);
	
	void destroyParticleGroup(const VkDevice& device);

	//void updateParticles(glm::vec3 pos, glm::vec3 color, glm::vec3 normal);

	void draw(VkPipelineLayout pipelineLayout, VkCommandBuffer commandBuffer);
private:

	std::unique_ptr<DataBuffer> m_ParticleBuffer;

	std::vector<Particle> m_Particles;

	Particle m_VertexConstant = { {} };
	//Particle* m_pParticleBuffer;
	int m_ParticleCount;
	void CreateParticleBuffer(VkPhysicalDevice physicalDevice, VkDevice device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue);
};