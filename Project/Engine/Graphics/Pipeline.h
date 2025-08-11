#pragma once
#include <vector>
#include <memory>
#include <vulkan\vulkan_core.h>

#include <Engine/Graphics/ShaderBase.h>
#include <Engine/Graphics/CommandBuffer.h>
#include <Engine/Graphics/CommandPool.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Graphics/UniformBufferObject.h>


struct PipelineConfig {
	VkRenderPass renderPass = VK_NULL_HANDLE;           // which render pass to build for
	VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Depth state
	bool enableDepthTest = true;
	bool enableDepthWrite = true;

	// Vertex input: if false -> fullscreen/no-VB style
	bool useVertexInput = true;

	// Push constants: keep off for post
	bool usePushConstants = true;

	// If provided, Pipeline will use these instead of ShaderBase’s layout & sets.
	VkDescriptorSetLayout externalSetLayout = VK_NULL_HANDLE;
	const std::vector<VkDescriptorSet>* externalSets = nullptr;

	// If true, Record() draws a fullscreen triangle (3 verts) instead of Scene.
	bool fullscreenTriangle = false;
};

class Pipeline
{
public:
	Pipeline();
	~Pipeline();
	void Destroy(const VkDevice& vkDevice);
	void Initialize(const std::string& vertexShaderPath,
		const std::string& fragmentShaderPath,
		const VkVertexInputBindingDescription vkVertexInputBindingDesc,
		std::vector<VkVertexInputAttributeDescription> vkVertexInputAttributeDesc,
		const PipelineConfig& config);
	void Initialize(const std::string& vertexShaderPath, const std::string& fragmentShaderPath, const VkVertexInputBindingDescription vkVertexInputBindingDesc, std::vector<VkVertexInputAttributeDescription> vkVertexInputAttributeDesc, VkPrimitiveTopology topology);
	void Record(uint32_t imageIndex, VkRenderPass renderPass, const std::vector<VkFramebuffer>& swapChainFramebuffers, VkExtent2D swapChainExtent, Scene& scene);

	void setUbo(const UniformBufferObject& ubo) { m_Ubo = ubo; }
	void updateDescriptorSets();
	static VkFormat findDepthFormat(VkPhysicalDevice& vkPhysicalDevice, VkDevice& vkDevice);
	VkImage getDepthImage() { return m_DepthImage; };
	VkDeviceMemory getDepthImageMemory() { return m_DepthImageMemory; };
	VkImageView getDepthImageView() { return m_DepthImageView; };

private:
	void drawScene(uint32_t imageIndex, VkRenderPass renderPass, const std::vector<VkFramebuffer>& swapChainFramebuffers, VkExtent2D swapChainExtent, Scene& scene);

	void CreatePipeline(VkDevice device, VkRenderPass renderPass, VkPrimitiveTopology topology);

	VkPipeline m_Pipeline3d;
	std::unique_ptr<ShaderBase> m_Shader;
	PipelineConfig m_Config{};                  // NEW
	bool m_UseExternalDescriptors = false;      // NEW
	VkPipelineVertexInputStateCreateInfo m_EmptyVI{}; // NEW
	const VkPipelineVertexInputStateCreateInfo* pickVI(); // NEW
	UniformBufferObject m_Ubo{};

	VkPipelineLayout m_PipelineLayout;
	void updateUniformBuffer(uint32_t currentImage, VkExtent2D swapChainExtent);

	VkPushConstantRange createPushConstantRange();

	void createImage(VkPhysicalDevice& vkPhysicalDevice, VkDevice& vkDevice,uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	
	static VkFormat findSupportedFormat(VkPhysicalDevice& vkPhysicalDevice, VkDevice& vkDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	void createDepthResources(VkPhysicalDevice& vkPhysicalDevice, VkDevice& vkDevice, VkExtent2D swapChainExtent);

	bool hasStencilComponent(VkFormat format);
	VkImageView createImageView(VkDevice& vkDevice, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	VkImage m_DepthImage;
	VkDeviceMemory m_DepthImageMemory;
	VkImageView m_DepthImageView;
};

