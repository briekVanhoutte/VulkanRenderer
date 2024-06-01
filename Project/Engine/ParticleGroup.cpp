#include "ParticleGroup.h"
#include "vulkanVars.h"

ParticleGroup::ParticleGroup(physx::PxVec4* particleBuffer, int ParticleCount, std::vector<Particle> particles)
	:m_ParticleCount(ParticleCount) , m_pParticleBuffer(particleBuffer), m_Particles(particles)
{
	auto& vulkan_vars = vulkanVars::GetInstance();

	m_ParticleCount = m_Particles.size();

	initialize(vulkan_vars.physicalDevice, vulkan_vars.device, vulkan_vars.commandPoolModelPipeline.m_CommandPool, vulkan_vars.graphicsQueue);
}

void ParticleGroup::initialize(VkPhysicalDevice physicalDevice, VkDevice device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue)
{
	VkDeviceSize vertexBufferSize = sizeof(m_Particles[0]) * m_Particles.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	auto VertexStagingBuffer = std::make_unique<DataBuffer>(physicalDevice, device,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		vertexBufferSize
	);

	VertexStagingBuffer->map(vertexBufferSize, m_Particles.data());

	m_ParticleBuffer = std::make_unique<DataBuffer>(physicalDevice, device,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vertexBufferSize
	);



	m_ParticleBuffer->copyBuffer(VertexStagingBuffer->getVkBuffer(), commandPool, device, vertexBufferSize, graphicsQueue);
	VertexStagingBuffer->destroy(device);



	/*VkDeviceSize particleBufferSizew = sizeof(physx::PxVec4) * m_ParticleCount;

	m_ParticleBuffer = std::make_unique<DataBuffer>(physicalDevice, device,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		particleBufferSizew
	);

	m_ParticleBuffer->uploadRaw(particleBufferSizew, m_pParticleBuffer);*/
}

void ParticleGroup::destroyParticleGroup(const VkDevice& device)
{
}


void ParticleGroup::draw(VkPipelineLayout pipelineLayout, VkCommandBuffer commandBuffer)
{
	m_ParticleBuffer->bindAsVertexBuffer(commandBuffer);

	vkCmdPushConstants(
		commandBuffer,
		pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT, // Stage flag should match the push constant range in the layout
		0,                          // Offset within the push constant block
		sizeof(m_VertexConstant),          // Size of the push constants to update
		&m_VertexConstant           
	);

	vkCmdDraw(commandBuffer, m_ParticleCount, 1, 0, 0);

	//vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_Indices.size()), 1, 0, 0, 0);
}

void ParticleGroup::CreateParticleBuffer(VkPhysicalDevice physicalDevice, VkDevice device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue)
{

}
