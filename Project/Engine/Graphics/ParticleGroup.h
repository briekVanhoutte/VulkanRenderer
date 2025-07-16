#pragma once

#include <vulkan\vulkan_core.h>
#include <glm/glm.hpp>
#include <Engine/Graphics/DataBuffer.h>

#include <Engine/Graphics/vulkanVars.h>
#include <Engine/Scene/MeshData.h>
#include <Engine/Graphics/Particle.h>
#include <memory>
#include <array>

#include <PxParticleBuffer.h>

class ParticleGroup {
public:
	ParticleGroup(physx::PxVec4* particleBuffer, int ParticleCount, const std::vector<Particle>& particles);
	void initialize(VkPhysicalDevice physicalDevice, VkDevice device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue);
	
	void setPosition(glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles);

	void destroyParticleGroup(const VkDevice& device);

	void draw(VkPipelineLayout pipelineLayout, VkCommandBuffer commandBuffer);
private:

	std::vector<Particle> m_Particles;
	std::array<std::unique_ptr<DataBuffer>, MAX_FRAMES_IN_FLIGHT> m_ParticleBuffers;

	MeshData m_VertexConstant = {  };
	physx::PxVec4* m_pParticleBuffer;
	int m_ParticleCount;
	void CreateParticleBuffer(VkPhysicalDevice physicalDevice, VkDevice device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue);

	void update();
};