#pragma once

#include <vulkan/vulkan_core.h>
#include <Engine/Graphics/CommandBuffer.h>
#include <Engine/Graphics/Vertex.h>
#include <Engine/Graphics/QueueFamilyIndices.h>

class CommandPool
{
public:
	CommandPool() {};
	VkCommandPool m_CommandPool{ VK_NULL_HANDLE };
	
	void initialize( const QueueFamilyIndices& queue);
	void destroy();

	CommandBuffer createCommandBuffer() const;
private:
	VkDevice m_VkDevice;
};