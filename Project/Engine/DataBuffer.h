#pragma once
#include <stdexcept>

#include "vulkanbase\VulkanUtil.h"
#include "vulkan\vulkan_core.h"

class DataBuffer
{
public:
	DataBuffer(
		VkPhysicalDevice physicalDevice,
		VkDevice device,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkDeviceSize size
	);

	~DataBuffer() {
	}

	void upload(VkDeviceSize size, void* data);
	void map(VkDeviceSize size, void* data);
	void remap(VkDeviceSize size, void* data);
	void destroy(const VkDevice& device);
	void bindAsVertexBuffer(VkCommandBuffer commandBuffer);
	void bindAsIndexBuffer(VkCommandBuffer commandBuffer);

	VkBuffer getVkBuffer();
	void* getUniformBuffer();
	VkBuffer m_VkBuffer;
	VkDeviceSize getSizeInBytes();
	VkDeviceMemory getBufferMemory() { return m_VkBufferMemory; }

	void copyBuffer(VkBuffer srcBuffer, const VkCommandPool& commandPool, const VkDevice& device, VkDeviceSize size, const VkQueue& graphicsQueue);

	static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
private:
	

	VkDevice m_VkDevice;
	VkDeviceSize m_Size;
	
	VkDeviceMemory m_VkBufferMemory;

	void* m_UniformBufferMapped;
};