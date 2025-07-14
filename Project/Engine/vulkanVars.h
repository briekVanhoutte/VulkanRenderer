#pragma once

#include "Engine/Singleton.h"
#include "Engine/CommandBuffer.h"
#include "Engine/CommandPool.h"
#include "Vulkanbase/VulkanUtil.h"

class vulkanVars : public Singleton<vulkanVars> {
public:
	CommandPool commandPoolModelPipeline{};
	CommandPool commandPoolParticlesPipeline{};
	VkDevice device = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	std::string vertexShaderPat;
	std::string fragmentShaderPath;
	VkRenderPass renderPass = VK_NULL_HANDLE;
	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkExtent2D swapChainExtent;
	std::vector<CommandBuffer> commandBuffers; 
	size_t currentFrame = 0;
};