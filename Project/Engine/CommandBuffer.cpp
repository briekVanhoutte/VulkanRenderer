#include "CommandBuffer.h"
#include <stdexcept>


void CommandBuffer::setVkCommandBuffer(VkCommandBuffer buffer) {
	m_VkCommandBuffer = buffer;
}

void CommandBuffer::reset()const {
	vkResetCommandBuffer(m_VkCommandBuffer, 0 /*VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT */ );
}
void CommandBuffer::beginRecording()const {
	VkCommandBufferBeginInfo begineInfo{};
	begineInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begineInfo.flags = 0;
	begineInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(m_VkCommandBuffer, &begineInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording commandbuffer!");
	}
}
void CommandBuffer::endRecording()const {
	if (vkEndCommandBuffer(m_VkCommandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to end recording commandbuffer!");
	}
}

void CommandBuffer::submit(VkSubmitInfo& info) const {
	info.commandBufferCount = 1;
	info.pCommandBuffers = &m_VkCommandBuffer;
}