#pragma once

#include "vulkan\vulkan_core.h"
#include "CommandBuffer.h"
#include "vulkanbase\VulkanUtil.h"

class CommandPool
{
public:
	CommandPool() {};
	VkCommandPool m_CommandPool{ VK_NULL_HANDLE };
	/*VkDevice m_VkDevice{ VK_NULL_HANDLE }
	{

	}*/
	
	void initialize( const QueueFamilyIndices& queue);
	void destroy();

	CommandBuffer createCommandBuffer() const;
private:
	VkDevice m_VkDevice;
};