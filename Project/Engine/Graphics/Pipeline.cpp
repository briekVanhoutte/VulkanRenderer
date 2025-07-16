#include "Pipeline.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <Engine/Graphics/vulkanVars.h>
#include <Engine/Scene/MeshData.h>
#include <iostream>



Pipeline::Pipeline()
{
	m_Ubo = {};
	m_Ubo.view = glm::mat4{ {1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1} };
	m_Ubo.proj = glm::mat4{ {1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1} };
	m_Ubo.proj[1][1] *= -1;
	
}

Pipeline::~Pipeline()
{
}


void Pipeline::Destroy(const VkDevice& vkDevice)
{
	vkDestroyPipeline(vkDevice, m_Pipeline3d, nullptr);
	
	vkDestroyPipelineLayout(vkDevice, m_PipelineLayout, nullptr);

	m_Shader->Destroy( vkDevice);

	vkDestroyImage(vkDevice, m_DepthImage, nullptr);
	vkFreeMemory(vkDevice, m_DepthImageMemory, nullptr);
	vkDestroyImageView(vkDevice, m_DepthImageView, nullptr);
}

void Pipeline::Initialize( const std::string& vertexShaderPath, const std::string& fragmentShaderPath, const VkVertexInputBindingDescription vkVertexInputBindingDesc, std::vector<VkVertexInputAttributeDescription> vkVertexInputAttributeDesc, VkPrimitiveTopology topology)
{
	auto& vulkan_vars = vulkanVars::GetInstance();

	m_Shader = std::make_unique<ShaderBase>(vertexShaderPath, fragmentShaderPath);
	m_Shader->initialize(vulkan_vars.physicalDevice, vulkan_vars.device, vkVertexInputBindingDesc, vkVertexInputAttributeDesc);
	m_Shader->createDescriptorSetLayout(vulkan_vars.device);

	createDepthResources(vulkan_vars.physicalDevice, vulkan_vars.device, vulkan_vars.swapChainExtent);

	CreatePipeline(vulkan_vars.device , vulkan_vars.renderPass, topology);
}

void Pipeline::Record(uint32_t imageIndex, VkRenderPass renderPass, const std::vector<VkFramebuffer>& swapChainFramebuffers, VkExtent2D swapChainExtent, Scene& scene)
{

	drawScene(imageIndex,renderPass,swapChainFramebuffers,swapChainExtent, scene);


	updateUniformBuffer(imageIndex, swapChainExtent);
}

void Pipeline::drawScene(uint32_t imageIndex, VkRenderPass renderPass,const std::vector<VkFramebuffer>& swapChainFramebuffers, VkExtent2D swapChainExtent, Scene& scene)
{
	auto& vulkan_vars = vulkanVars::GetInstance();
	size_t currentSlice = vulkan_vars.currentFrame % MAX_FRAMES_IN_FLIGHT;
	vkCmdBindPipeline(vulkan_vars.commandBuffers[currentSlice].m_VkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline3d);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(vulkan_vars.commandBuffers[currentSlice].m_VkCommandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;
	vkCmdSetScissor(vulkan_vars.commandBuffers[currentSlice].m_VkCommandBuffer, 0, 1, &scissor);

	

	//vkCmdBindPipeline(vulkan_vars.commandBuffer.m_VkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline3d);

	m_Shader->bindDescriptorSet(vulkan_vars.commandBuffers[currentSlice].m_VkCommandBuffer, m_PipelineLayout, imageIndex);

	assert(m_Shader);
	assert(m_Shader->getVertexShaderStageInfo().module != VK_NULL_HANDLE);
	auto& vertexInput = m_Shader->getVertexInputStateInfo();

	scene.drawScene(m_PipelineLayout, vulkan_vars.commandBuffers[currentSlice].m_VkCommandBuffer);

	
}

void Pipeline::CreatePipeline(VkDevice device, VkRenderPass renderPass, VkPrimitiveTopology topology) {
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_Shader->getDescriptorSetLayout();
	
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	VkPushConstantRange pushConstantRange = createPushConstantRange();
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};

	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;


	VkPipelineShaderStageCreateInfo vertShaderStageInfo = m_Shader->getVertexShaderStageInfo();
	VkPipelineShaderStageCreateInfo fragShaderStageInfo = m_Shader->getFragmentShaderStageInfo();

	VkPipelineShaderStageCreateInfo shaderStages[] = {
		vertShaderStageInfo,
		fragShaderStageInfo
	};

	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;


	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = m_PipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	pipelineInfo.pVertexInputState = &m_Shader->getVertexInputStateInfo();
	pipelineInfo.pInputAssemblyState = &m_Shader->getInputAssemblyStateInfo(topology);



	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;

	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional

	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional


	pipelineInfo.pDepthStencilState = &depthStencil;


	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline3d) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	auto& vertexInput = m_Shader->getVertexInputStateInfo();

	std::cout << " === Vertex Input Check ===\n";
	std::cout << "bindingDesc.binding: " << vertexInput.pVertexBindingDescriptions[0].binding << "\n";
	std::cout << "bindingDesc.stride: " << vertexInput.pVertexBindingDescriptions[0].stride << "\n";
	std::cout << "bindingDesc.inputRate: " << vertexInput.pVertexBindingDescriptions[0].inputRate << "\n";


	vkDestroyShaderModule(device, vertShaderStageInfo.module, nullptr);
	vkDestroyShaderModule(device, fragShaderStageInfo.module, nullptr);
}



void Pipeline::updateUniformBuffer(uint32_t currentImage, VkExtent2D swapChainExtent)
{
	m_Ubo.proj[1][1] *= -1;
	m_Shader->updateUniformBuffer(currentImage, m_Ubo);
}

VkPushConstantRange Pipeline::createPushConstantRange()
{
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Stage the push constant is accessible from
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(MeshData); // Size of push constant block

	return pushConstantRange;
}


void Pipeline::createImage(VkPhysicalDevice& vkPhysicalDevice, VkDevice& vkDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(vkDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements{};
	vkGetImageMemoryRequirements(vkDevice, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = DataBuffer::findMemoryType(vkPhysicalDevice, memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(vkDevice, image, imageMemory, 0);
}

VkFormat Pipeline::findSupportedFormat(VkPhysicalDevice& vkPhysicalDevice, VkDevice& vkDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates) {
		VkFormatProperties props{};
		vkGetPhysicalDeviceFormatProperties(vkPhysicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
	return VkFormat();
}

void Pipeline::createDepthResources(VkPhysicalDevice& vkPhysicalDevice, VkDevice& vkDevice, VkExtent2D swapChainExtent)
{
	VkFormat depthFormat = findDepthFormat(vkPhysicalDevice, vkDevice);

	createImage(vkPhysicalDevice,vkDevice, swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage, m_DepthImageMemory);

	m_DepthImageView = createImageView(vkDevice, m_DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

VkFormat Pipeline::findDepthFormat(VkPhysicalDevice& vkPhysicalDevice, VkDevice& vkDevice)
{
	return findSupportedFormat(vkPhysicalDevice, vkDevice,
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

bool Pipeline::hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkImageView Pipeline::createImageView(VkDevice& vkDevice, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView{};
	if (vkCreateImageView(vkDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}
