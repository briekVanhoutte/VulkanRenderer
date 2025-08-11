#include "Pipeline.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <Engine/Graphics/vulkanVars.h>
#include <Engine/Graphics/MeshData.h>
#include <iostream>



Pipeline::Pipeline()
{
	m_Ubo = {};
	m_Ubo.view = glm::mat4{ {1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1} };
	m_Ubo.proj = glm::mat4{ {1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1} };
	//m_Ubo.proj[1][1] *= -1;

	m_DepthImage = VK_NULL_HANDLE;
	m_DepthImageMemory = VK_NULL_HANDLE;
	m_DepthImageView = VK_NULL_HANDLE;
	m_Pipeline3d = VK_NULL_HANDLE;
	m_PipelineLayout = VK_NULL_HANDLE;

}

Pipeline::~Pipeline()
{
}


void Pipeline::Destroy(const VkDevice& vkDevice)
{
	vkDestroyPipeline(vkDevice, m_Pipeline3d, nullptr);

	vkDestroyPipelineLayout(vkDevice, m_PipelineLayout, nullptr);

	m_Shader->Destroy(vkDevice);

	vkDestroyImage(vkDevice, m_DepthImage, nullptr);
	vkFreeMemory(vkDevice, m_DepthImageMemory, nullptr);
	vkDestroyImageView(vkDevice, m_DepthImageView, nullptr);
}

void Pipeline::Initialize(const std::string& vertexShaderPath,
	const std::string& fragmentShaderPath,
	const VkVertexInputBindingDescription vkVertexInputBindingDesc,
	std::vector<VkVertexInputAttributeDescription> vkVertexInputAttributeDesc,
	const PipelineConfig& config)
{
	auto& vulkan_vars = vulkanVars::GetInstance();
	m_Config = config;
	m_UseExternalDescriptors = (m_Config.externalSetLayout != VK_NULL_HANDLE &&
		m_Config.externalSets != nullptr);

	// Shader setup (unchanged)
	m_Shader = std::make_unique<ShaderBase>(vertexShaderPath, fragmentShaderPath);
	// You can still pass the mesh vertex layout even if we end up not using it (safe):
	m_Shader->initialize(vulkan_vars.physicalDevice, vulkan_vars.device,
		vkVertexInputBindingDesc, vkVertexInputAttributeDesc);

	// Only create the default descriptor set layout when we’re NOT using external sets
	if (!m_UseExternalDescriptors) {
		m_Shader->createDescriptorSetLayout(vulkan_vars.device);
	}

	// Depth resources are only needed when depth is enabled (offscreen 3D)
	if (m_Config.enableDepthTest || m_Config.enableDepthWrite) {
		createDepthResources(vulkan_vars.physicalDevice, vulkan_vars.device, vulkan_vars.swapChainExtent);
	}

	// Build the pipeline using the target render pass from the config
	CreatePipeline(vulkan_vars.device,
		m_Config.renderPass != VK_NULL_HANDLE ? m_Config.renderPass
		: vulkan_vars.renderPass,
		m_Config.topology);
}

void Pipeline::Initialize(const std::string& vertexShaderPath,
	const std::string& fragmentShaderPath,
	const VkVertexInputBindingDescription vkVertexInputBindingDesc,
	std::vector<VkVertexInputAttributeDescription> vkVertexInputAttributeDesc,
	VkPrimitiveTopology topology)
{
	PipelineConfig def; // defaults replicate old behavior
	def.renderPass = vulkanVars::GetInstance().renderPass;
	def.topology = topology;
	def.enableDepthTest = true;
	def.enableDepthWrite = true;
	def.useVertexInput = true;
	def.usePushConstants = true;
	def.fullscreenTriangle = false;

	Initialize(vertexShaderPath, fragmentShaderPath, vkVertexInputBindingDesc,
		std::move(vkVertexInputAttributeDesc), def);
}

const VkPipelineVertexInputStateCreateInfo* Pipeline::pickVI()
{
	if (m_Config.useVertexInput) {
		return &m_Shader->getVertexInputStateInfo();
	}
	else {
		// static empty VI (legal for fullscreen triangle)
		const_cast<Pipeline*>(this)->m_EmptyVI = {};
		m_EmptyVI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		m_EmptyVI.vertexBindingDescriptionCount = 0;
		m_EmptyVI.pVertexBindingDescriptions = nullptr;
		m_EmptyVI.vertexAttributeDescriptionCount = 0;
		m_EmptyVI.pVertexAttributeDescriptions = nullptr;
		return &m_EmptyVI;
	}
}

void Pipeline::Record(uint32_t imageIndex, VkRenderPass renderPass, const std::vector<VkFramebuffer>& swapChainFramebuffers, VkExtent2D swapChainExtent, Scene& scene)
{

	drawScene(imageIndex, renderPass, swapChainFramebuffers, swapChainExtent, scene);


	//updateUniformBuffer(imageIndex, swapChainExtent);
}

void Pipeline::drawScene(uint32_t imageIndex, VkRenderPass renderPass, const std::vector<VkFramebuffer>& swapChainFramebuffers, VkExtent2D swapChainExtent, Scene& scene)
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

	if (m_UseExternalDescriptors) {
		const auto& sets = *m_Config.externalSets;
		if (imageIndex < sets.size()) {
			vkCmdBindDescriptorSets(vulkan_vars.commandBuffers[currentSlice].m_VkCommandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
				0, 1, &sets[imageIndex], 0, nullptr);
		}
	}
	else {
		m_Shader->bindDescriptorSet(vulkan_vars.commandBuffers[currentSlice].m_VkCommandBuffer,
			m_PipelineLayout, imageIndex);
	}

	if (m_Config.fullscreenTriangle) {
		// Post-process: no meshes, just a fullscreen tri
		vkCmdDraw(vulkan_vars.commandBuffers[currentSlice].m_VkCommandBuffer, 3, 1, 0, 0);
	}
	else {
		// Normal path: draw your scene (unchanged)
		scene.drawScene(m_PipelineLayout, vulkan_vars.commandBuffers[currentSlice].m_VkCommandBuffer);
		// Only update UBOs for the normal path
		updateUniformBuffer(imageIndex, swapChainExtent);
	}

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
	if (m_Config.fullscreenTriangle) {
		rasterizer.cullMode = VK_CULL_MODE_NONE;   // no culling for post
	}
	else {
		rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	}

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


	VkDescriptorSetLayout setLayout = m_UseExternalDescriptors ? m_Config.externalSetLayout : m_Shader->getDescriptorSetLayout();


	// Push constants on/off
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &setLayout;

	VkPushConstantRange pushConstantRange{};
	if (m_Config.usePushConstants) {
		pushConstantRange = createPushConstantRange();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
	}
	else {
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;
	}

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	// Shader stages from ShaderBase (same as before)
	auto vertShaderStageInfo = m_Shader->getVertexShaderStageInfo();
	auto fragShaderStageInfo = m_Shader->getFragmentShaderStageInfo();
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// Vertex Input: select empty or mesh VI
	const VkPipelineVertexInputStateCreateInfo* viPtr = pickVI();

	// Depth state: dynamic per config
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = m_Config.enableDepthTest ? VK_TRUE : VK_FALSE;
	depthStencil.depthWriteEnable = m_Config.enableDepthWrite ? VK_TRUE : VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	// Input Assembly still comes from ShaderBase (ok), but use the chosen topology
	auto ia = m_Shader->getInputAssemblyStateInfo(m_Config.topology);


	VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = viPtr;
	pipelineInfo.pInputAssemblyState = &ia;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil; // <-- now dynamic
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = m_PipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;


	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline3d) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	//auto& vertexInput = m_Shader->getVertexInputStateInfo();

	vkDestroyShaderModule(device, vertShaderStageInfo.module, nullptr);
	vkDestroyShaderModule(device, fragShaderStageInfo.module, nullptr);
}



void Pipeline::updateUniformBuffer(uint32_t currentImage, VkExtent2D swapChainExtent)
{
	//m_Ubo.proj[1][1] *= -1;
	m_Shader->updateUniformBuffer(currentImage, m_Ubo);
}

VkPushConstantRange Pipeline::createPushConstantRange()
{
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(MeshData);

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
	for (VkFormat fmt : candidates) {
		VkFormatProperties props{};
		vkGetPhysicalDeviceFormatProperties(vkPhysicalDevice, fmt, &props);
		const auto need = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT |
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
		if ((props.optimalTilingFeatures & need) == need)
			return fmt;
	}
	throw std::runtime_error("No sampleable depth format found");
}

void Pipeline::createDepthResources(VkPhysicalDevice& vkPhysicalDevice, VkDevice& vkDevice, VkExtent2D swapChainExtent)
{
	VkFormat depthFormat = findDepthFormat(vkPhysicalDevice, vkDevice);

	createImage(vkPhysicalDevice, vkDevice, swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage, m_DepthImageMemory);

	m_DepthImageView = createImageView(vkDevice, m_DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void Pipeline::updateDescriptorSets()
{
	m_Shader->updateDescriptorSet();
}

VkFormat Pipeline::findDepthFormat(VkPhysicalDevice& vkPhysicalDevice, VkDevice& vkDevice)
{
	return findSupportedFormat(vkPhysicalDevice, vkDevice,
		{ VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_X8_D24_UNORM_PACK32, // often sampleable, but less universal than D32
		VK_FORMAT_D16_UNORM },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT
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
