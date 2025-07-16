#pragma once

#include <vector>
#include <memory>
#include <Engine/Graphics/DataBuffer.h>

class DescriptorPool
{
public:
	DescriptorPool() {};
	DescriptorPool(const VkDevice& device, VkDeviceSize size, size_t count);
	void Initialize(const VkDevice& device);

	void Destroy(const VkDevice& device);

	const VkDescriptorSetLayout& getDescriptorSetLayout()
	{
		return m_DescriptorSetLayout;
	}

	~DescriptorPool() ;
	void createDescriptorSets(std::vector<VkBuffer> buffers);

	void bindDescriptorSet(VkCommandBuffer buffer, VkPipelineLayout layout, size_t index);

private:
	VkDevice m_Device;
	VkDeviceSize m_Size;
	VkDescriptorPool m_DescriptorPool;
	std::vector<VkDescriptorSet> m_DescriptorSets;
	size_t m_Count;
	VkDescriptorSetLayout m_DescriptorSetLayout;
};