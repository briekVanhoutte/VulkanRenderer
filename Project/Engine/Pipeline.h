#include <vector>
#include <memory>
#include <vulkan\vulkan_core.h>
#include "vulkanbase\VulkanUtil.h"
#include "Engine\ShaderBase.h"
#include "Engine\CommandBuffer.h"
#include "Engine\CommandPool.h"
#include "Engine\Scene.h"

class Pipeline
{
public:
	Pipeline();
	~Pipeline();
	void Destroy(const VkDevice& vkDevice);

	void Initialize( VkPhysicalDevice& vkPhysicalDevice, VkDevice& vkDevice, const std::string& vertexShaderPath, const std::string& fragmentShaderPath, VkRenderPass renderPass, const VkQueue& graphicsQueue, const VkExtent2D& swapChainExtent);
	void Record(uint32_t imageIndex, VkRenderPass renderPass, const std::vector<VkFramebuffer>& swapChainFramebuffers, VkExtent2D swapChainExtent, Scene& scene);
	//CommandBuffer m_Buffer;

	void setUbo(const UniformBufferObject& ubo) { m_Ubo = ubo; }

	static VkFormat findDepthFormat(VkPhysicalDevice& vkPhysicalDevice, VkDevice& vkDevice);
	VkImage getDepthImage() { return m_DepthImage; };
	VkDeviceMemory getDepthImageMemory() { return m_DepthImageMemory; };
	VkImageView getDepthImageView() { return m_DepthImageView; };

private:
	void drawScene(uint32_t imageIndex, VkRenderPass renderPass, const std::vector<VkFramebuffer>& swapChainFramebuffers, VkExtent2D swapChainExtent, Scene& scene);

	void CreatePipeline(VkDevice device, VkRenderPass renderPass);


	VkPipeline m_Pipeline3d;
	std::unique_ptr<ShaderBase> m_Shader;

	UniformBufferObject m_Ubo;

	VkPipelineLayout m_PipelineLayout;
	void updateUniformBuffer(uint32_t currentImage, VkExtent2D swapChainExtent);

	VkPushConstantRange createPushConstantRange();


	// depth buffer

	void createImage(VkPhysicalDevice& vkPhysicalDevice, VkDevice& vkDevice,uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	
	static VkFormat findSupportedFormat(VkPhysicalDevice& vkPhysicalDevice, VkDevice& vkDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	void createDepthResources(VkPhysicalDevice& vkPhysicalDevice, VkDevice& vkDevice, VkExtent2D swapChainExtent);

	bool hasStencilComponent(VkFormat format);
	VkImageView createImageView(VkDevice& vkDevice, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	VkImage m_DepthImage;
	VkDeviceMemory m_DepthImageMemory;
	VkImageView m_DepthImageView;
};

