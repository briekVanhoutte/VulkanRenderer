#include "ParticleGroup.h"
#include <Engine/Graphics/vulkanVars.h>
#include <Engine/Physics/PhysxBase.h>

#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp> 
#include <glm/ext/matrix_clip_space.hpp> 
#include <glm/ext/scalar_constants.hpp> 

ParticleGroup::ParticleGroup(physx::PxVec4* particleBuffer, int ParticleCount,const std::vector<Particle>& particles)
	:m_ParticleCount(ParticleCount) , m_pParticleBuffer(particleBuffer), m_Particles(particles)
{
	auto& vulkan_vars = vulkanVars::GetInstance();

	m_ParticleCount = m_Particles.size();

	m_VertexConstant = {};
	m_VertexConstant.model = glm::mat4{ {1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1} };

	initialize(vulkan_vars.physicalDevice, vulkan_vars.device, vulkan_vars.commandPoolModelPipeline.m_CommandPool, vulkan_vars.graphicsQueue);
}

void ParticleGroup::initialize(VkPhysicalDevice physicalDevice, VkDevice device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue)
{
	VkDeviceSize vertexBufferSize = sizeof(m_Particles[0]) * m_Particles.size();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		auto VertexStagingBuffer = std::make_unique<DataBuffer>(physicalDevice, device,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			vertexBufferSize
		);

		VertexStagingBuffer->map(vertexBufferSize, m_Particles.data());

		m_ParticleBuffers[i] = std::make_unique<DataBuffer>(physicalDevice, device,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			vertexBufferSize
		);



		m_ParticleBuffers[i]->copyBuffer(VertexStagingBuffer->getVkBuffer(), commandPool, device, vertexBufferSize, graphicsQueue);
		VertexStagingBuffer->destroy(device);

	}
}

void ParticleGroup::setPosition(glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles)
{
	glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), position);

	glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngles.x), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotationAngles.y), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotationAngles.z), glm::vec3(0.0f, 0.0f, 1.0f));

	glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

	m_VertexConstant.model = translationMatrix * rotationMatrix * scaleMatrix;
}

void ParticleGroup::destroyParticleGroup(const VkDevice& device)
{
	auto& vulkan_vars = vulkanVars::GetInstance();
	m_ParticleBuffers[vulkan_vars.currentFrame%MAX_FRAMES_IN_FLIGHT]->destroy(device);
}


void ParticleGroup::draw(VkPipelineLayout pipelineLayout, VkCommandBuffer commandBuffer)
{

	update();
	auto& vulkan_vars = vulkanVars::GetInstance();

	m_ParticleBuffers[vulkan_vars.currentFrame % MAX_FRAMES_IN_FLIGHT]->bindAsVertexBuffer(commandBuffer);

	vkCmdPushConstants(
		commandBuffer,
		pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		0,                       
		sizeof(m_VertexConstant),
		&m_VertexConstant           
	);

	vkCmdDraw(commandBuffer, m_ParticleCount, 1, 0, 0);
}

void ParticleGroup::CreateParticleBuffer(VkPhysicalDevice physicalDevice, VkDevice device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue)
{

}

void ParticleGroup::update()
{
	auto& physx_base = PhysxBase::GetInstance();
	auto& vulkan_vars = vulkanVars::GetInstance();
	m_Particles = physx_base.getParticles();
	m_ParticleCount = m_Particles.size();

	VkDeviceSize vertexBufferSize = sizeof(m_Particles[0]) * m_Particles.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	auto VertexStagingBuffer = std::make_unique<DataBuffer>(vulkan_vars.physicalDevice, vulkan_vars.device,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		vertexBufferSize
	);

	VertexStagingBuffer->map(vertexBufferSize, m_Particles.data());

	m_ParticleBuffers[vulkan_vars.currentFrame % MAX_FRAMES_IN_FLIGHT]->destroy(vulkan_vars.device);

	m_ParticleBuffers[vulkan_vars.currentFrame % MAX_FRAMES_IN_FLIGHT] = std::make_unique<DataBuffer>(vulkan_vars.physicalDevice, vulkan_vars.device,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vertexBufferSize
	);



	m_ParticleBuffers[vulkan_vars.currentFrame % MAX_FRAMES_IN_FLIGHT]->copyBuffer(VertexStagingBuffer->getVkBuffer(), vulkan_vars.commandPoolModelPipeline.m_CommandPool, vulkan_vars.device, vertexBufferSize, vulkan_vars.graphicsQueue);
	VertexStagingBuffer->destroy(vulkan_vars.device);



}
