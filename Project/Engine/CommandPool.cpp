#include "CommandPool.h"


CommandBuffer CommandPool::createCommandBuffer() const {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_CommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;
	VkCommandBuffer buffer;
	if (vkAllocateCommandBuffers(m_VkDevice, &allocInfo, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
	
	CommandBuffer cBuffer = CommandBuffer{};
	cBuffer.setVkCommandBuffer(buffer);
	return cBuffer;
}

void CommandPool::initialize(const VkDevice& device, const QueueFamilyIndices& queue) {
	//QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
	m_VkDevice = device;

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queue.graphicsFamily.value();

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}

	
}

void CommandPool::destroy() {
	vkDestroyCommandPool(m_VkDevice, m_CommandPool, nullptr);
}