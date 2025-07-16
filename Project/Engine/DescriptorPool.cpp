#include "DescriptorPool.h"
#include <iostream>
#include <Engine/vulkanVars.h>

DescriptorPool::DescriptorPool(const VkDevice& device, VkDeviceSize size, size_t count)
	:m_Device{device}, m_Size{size},m_Count{count}
{
	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(count);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = count;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void DescriptorPool::Initialize(const VkDevice& device)
{
	// create discription set layout
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void DescriptorPool::Destroy(const VkDevice& device)
{
	vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);

	vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
}

DescriptorPool::~DescriptorPool()
{

}

void DescriptorPool::createDescriptorSets(std::vector<VkBuffer> buffers)
{
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_DescriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_DescriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	m_DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(m_Device, &allocInfo, m_DescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	size_t descriptorIndex = 0;
	for (auto& buf : buffers)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = buf;
		bufferInfo.offset = 0;
		bufferInfo.range = m_Size;

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_DescriptorSets[descriptorIndex];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(m_Device, 1, &descriptorWrite, 0, nullptr);
		++descriptorIndex;
	}
}

void DescriptorPool::bindDescriptorSet(VkCommandBuffer buffer, VkPipelineLayout layout, size_t index)
{
	if (index >= m_DescriptorSets.size())
	{
		std::cout << "out of range\n";
		return;
	}
	vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &m_DescriptorSets[index], 0, nullptr);
}
