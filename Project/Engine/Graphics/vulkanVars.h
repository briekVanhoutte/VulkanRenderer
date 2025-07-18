#pragma once

#include <Engine/Core/Singleton.h>
#include <Engine/Graphics/CommandBuffer.h>
#include <Engine/Graphics/CommandPool.h>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 3;
const int MAX_TEXTURES = 32;

class vulkanVars : public Singleton<vulkanVars> {
public:
	CommandPool commandPoolModelPipeline{};
	CommandPool commandPoolParticlesPipeline{};
	VkDevice device = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkRenderPass renderPass = VK_NULL_HANDLE;
	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkExtent2D swapChainExtent;
	std::vector<CommandBuffer> commandBuffers; 
	size_t currentFrame = 0;
};


