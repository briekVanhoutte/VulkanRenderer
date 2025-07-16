#include "CommandPool.h"
#include <Engine/Graphics/vulkanVars.h>
#include <stdexcept>

CommandBuffer CommandPool::createCommandBuffer() const {
	auto& vulkan_vars = vulkanVars::GetInstance();
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_CommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;
	VkCommandBuffer buffer;
	if (vkAllocateCommandBuffers(vulkan_vars.device, &allocInfo, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
	
	CommandBuffer cBuffer = CommandBuffer{};
	cBuffer.setVkCommandBuffer(buffer);
	return cBuffer;
}

void CommandPool::initialize( const QueueFamilyIndices& queue) {
	auto& vulkan_vars = vulkanVars::GetInstance();

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queue.graphicsFamily.value();

	if (vkCreateCommandPool(vulkan_vars.device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}

	
}

void CommandPool::destroy() {
	auto& vulkan_vars = vulkanVars::GetInstance();
	vkDestroyCommandPool(vulkan_vars.device, m_CommandPool, nullptr);
}