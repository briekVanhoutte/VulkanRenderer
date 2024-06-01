#include "DataBuffer.h"
#include <iostream>

DataBuffer::DataBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkDeviceSize size)
	:m_VkDevice{ device }, m_Size{size}
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &m_VkBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create vertex buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, m_VkBuffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &m_VkBufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate vertex buffer memory!");
	}

	//vkMapMemory(device, m_VkBufferMemory, 0, size, 0, &m_UniformBufferMapped);

	vkBindBufferMemory(device, m_VkBuffer, m_VkBufferMemory, 0);
}


void DataBuffer::copyBuffer(VkBuffer srcBuffer, const VkCommandPool& commandPool, const VkDevice& device, VkDeviceSize size,const VkQueue& graphicsQueue) {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, m_VkBuffer, 1, &copyRegion);
	
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void DataBuffer::upload(VkDeviceSize size, void* data)
{
	vkMapMemory(m_VkDevice, m_VkBufferMemory, 0, size, 0, &m_UniformBufferMapped);
	memcpy(m_UniformBufferMapped, data, (size_t)size);
}

void DataBuffer::uploadRaw(VkDeviceSize size, void* data)
{
	vkMapMemory(m_VkDevice, m_VkBufferMemory, 0, size, 0, &m_UniformBufferMapped);
	m_UniformBufferMapped = data;
}

void DataBuffer::map(VkDeviceSize size, void* data)
{
	vkMapMemory(m_VkDevice, m_VkBufferMemory, 0, size, 0, &m_UniformBufferMapped);
	memcpy(m_UniformBufferMapped, data, (size_t)size);
	vkUnmapMemory(m_VkDevice, m_VkBufferMemory);
}

void DataBuffer::remap(VkDeviceSize size, void* data)
{
	memcpy(m_UniformBufferMapped, data, (size_t)size);
}

void DataBuffer::destroy(const VkDevice& device)
{
	vkDestroyBuffer(m_VkDevice, m_VkBuffer,nullptr);
	m_VkBuffer = VK_NULL_HANDLE;
	vkFreeMemory(m_VkDevice, m_VkBufferMemory, nullptr);
	m_VkBufferMemory = VK_NULL_HANDLE;
}

void DataBuffer::bindAsVertexBuffer(VkCommandBuffer commandBuffer) {
	VkBuffer vertexBuffers[] = { m_VkBuffer };
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
}

void DataBuffer::bindAsIndexBuffer(VkCommandBuffer commandBuffer)
{
	vkCmdBindIndexBuffer(commandBuffer, m_VkBuffer, 0, VK_INDEX_TYPE_UINT16);
}

VkBuffer DataBuffer::getVkBuffer()
{
	return m_VkBuffer;
}

void* DataBuffer::getUniformBuffer()
{
	return m_UniformBufferMapped;
}

VkDeviceSize DataBuffer::getSizeInBytes()
{
	return m_Size;
}

uint32_t DataBuffer::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}
