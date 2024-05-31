#pragma once
#include "vulkan\vulkan_core.h"

class CommandBuffer {
public:
	CommandBuffer() {};

	void setVkCommandBuffer( VkCommandBuffer buffer);
	VkCommandBuffer m_VkCommandBuffer;;
	void reset()const;
	void beginRecording()const;
	void endRecording()const;

	void submit(VkSubmitInfo& info) const;
private:
};