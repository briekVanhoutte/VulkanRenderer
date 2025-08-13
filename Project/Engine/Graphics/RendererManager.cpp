#include "RendererManager.h"
#include <Engine/Graphics/vulkanVars.h>
#include  <Engine/Core/WindowManager.h>
#include <set>
#include <algorithm>
#include <Engine/Graphics/Particle.h>
#include <Engine/Platform/Windows/VulkanSurface_Windows.h>
#include <Engine/Graphics/MaterialManager.h>
#include <Engine/Scene/MeshScene.h>
#include <Engine/Scene/SceneModelManager.h>

RendererManager::RendererManager() {
}

RendererManager::~RendererManager() {
}

static inline const char* VkResStr(VkResult r) {
	switch (r) {
	case VK_SUCCESS: return "VK_SUCCESS";
	case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
	case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
	default: return "OTHER_VK_ERROR";
	}
}

static VkImageView createView(VkDevice device, VkImage img, VkFormat fmt, VkImageAspectFlags aspect) {
	VkImageViewCreateInfo vi{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	vi.image = img; vi.viewType = VK_IMAGE_VIEW_TYPE_2D; vi.format = fmt;
	vi.subresourceRange.aspectMask = aspect; vi.subresourceRange.levelCount = 1; vi.subresourceRange.layerCount = 1;
	VkImageView view{};
	if (vkCreateImageView(device, &vi, nullptr, &view) != VK_SUCCESS) throw std::runtime_error("createView failed");
	return view;
}

static void createImage2D(VkPhysicalDevice phys, VkDevice dev, uint32_t w, uint32_t h, VkFormat fmt,
	VkImageUsageFlags usage, VkImage& img, VkDeviceMemory& mem) {
	VkImageCreateInfo ci{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	ci.imageType = VK_IMAGE_TYPE_2D; ci.extent = { w, h, 1 }; ci.mipLevels = 1; ci.arrayLayers = 1;
	ci.format = fmt; ci.tiling = VK_IMAGE_TILING_OPTIMAL; ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ci.usage = usage; ci.samples = VK_SAMPLE_COUNT_1_BIT; ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateImage(dev, &ci, nullptr, &img) != VK_SUCCESS) throw std::runtime_error("createImage2D failed");

	VkMemoryRequirements req{}; vkGetImageMemoryRequirements(dev, img, &req);
	VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	ai.allocationSize = req.size;
	ai.memoryTypeIndex = DataBuffer::findMemoryType(phys, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (vkAllocateMemory(dev, &ai, nullptr, &mem) != VK_SUCCESS) throw std::runtime_error("alloc image mem failed");
	vkBindImageMemory(dev, img, mem, 0);
}

void RendererManager::Initialize() {
	auto& vulkan_vars = vulkanVars::GetInstance();
	createInstance();
	setupDebugMessenger();
	createSurface();

	pickPhysicalDevice();
	createLogicalDevice();

	createSwapChain();
	createImageViews();

	createRenderPasses();


	vulkan_vars.commandPoolModelPipeline.initialize(findQueueFamilies(vulkan_vars.physicalDevice));
	vulkan_vars.commandPoolParticlesPipeline.initialize(findQueueFamilies(vulkan_vars.physicalDevice));

	// Create offscreen color images (one per swap image)
	createOffscreenTargets();
	createOffscreenDepthTargets();

	// Post descriptors (bind offscreen[i] to set=0/binding=0)
	createPostDescriptors();

	initPipeLines();

	vulkan_vars.commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vulkan_vars.commandBuffers[i] = vulkan_vars.commandPoolModelPipeline.createCommandBuffer();
	}

	//createFrameBuffers();

	createFramebuffersOffscreen();
	createFramebuffersPresent();



	createSyncObjects();
	//writePostDescriptors();
	setupStages();
}

void RendererManager::RenderFrame(const std::vector<RenderItem>& renderItems, Camera& camera) {
	auto& vk = vulkanVars::GetInstance();
	const size_t frameIndex = vk.currentFrame % MAX_FRAMES_IN_FLIGHT;

	// 1) Wait previous frame fence (do NOT reset yet)
	(vkWaitForFences(vk.device, 1, &inFlightFences[frameIndex], VK_TRUE, UINT64_MAX));

	// 2) Acquire
	uint32_t imageIndex = 0;
	VkResult ar = vkAcquireNextImageKHR(
		vk.device, swapChain, UINT64_MAX,
		imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex);

	if (ar == VK_ERROR_OUT_OF_DATE_KHR) {
		// No submit this frame; leave fence signaled from last frame.
		// Recreate swapchain and return.
		// recreateSwapchain();
		return;
	}
	if (ar != VK_SUCCESS && ar != VK_SUBOPTIMAL_KHR) {
		fprintf(stderr, "vkAcquireNextImageKHR failed: %s\n", VkResStr(ar));
		// Do NOT submit anything here. Fence is still signaled from last frame, so next loop won’t deadlock.
		return;
	}

	// 3) If this image was used before, wait the fence that submitted work for it
	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
		(vkWaitForFences(vk.device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX));
	}
	imagesInFlight[imageIndex] = inFlightFences[frameIndex]; // hand off

	// 4) Update UBOs …
	UniformBufferObject vp{};
	vp.view = glm::inverse(camera.CalculateCameraToWorld());
	vp.proj = glm::perspectiveRH_ZO(glm::radians(camera.fovAngle), camera.aspectRatio, camera.nearPlane, camera.farPlane);
	vp.proj[1][1] *= -1.0f;
	vp.cameraPos = camera.origin;
	m_Pipeline3d.setUbo(vp);
	m_PipelineParticles.setUbo(vp);

	float renderDistance = m_RenderDistance; // add a member, e.g. default 200.0f
	SceneModelManager::getInstance().setFrameView(vp.cameraPos, renderDistance);

	auto& texMgr = TextureManager::GetInstance();
	if (texMgr.isTextureListDirty()) { m_Pipeline3d.updateDescriptorSets(); texMgr.clearTextureListDirty(); }

	// 5) Record (frameIndex CB, imageIndex FB)
	vk.commandBuffers[frameIndex].reset();
	vk.commandBuffers[frameIndex].beginRecording();

	for (const RenderStage& stage : m_RenderStages) {
		VkRenderPassBeginInfo begin{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		begin.renderPass = stage.renderPass;
		begin.framebuffer = stage.framebuffers->at(imageIndex);
		begin.renderArea = { {0,0}, vk.swapChainExtent };

		VkClearValue clear[2];
		uint32_t cc = 0;
		clear[cc++].color = { {0.f,0.f,0.f,1.f} };
		if (stage.hasDepth) { clear[cc].depthStencil = { 1.f, 0 }; ++cc; }
		begin.clearValueCount = cc; begin.pClearValues = clear;

		vkCmdBeginRenderPass(vk.commandBuffers[frameIndex].m_VkCommandBuffer, &begin, VK_SUBPASS_CONTENTS_INLINE);

		for (Pipeline* p : stage.pipelines) {
			if (p == &m_PipelinePostProcess) continue;
			for (const RenderItem& item : renderItems) {

				if (p == &m_Pipeline3d && item.pipelineIndex == 0)
				{
					p->Record(imageIndex, stage.renderPass, *stage.framebuffers, vk.swapChainExtent, *item.scene);

				}
				if (m_EnableNormals && p == &m_PipelineNormals && item.pipelineIndex == 0)
				{
					p->Record(imageIndex, stage.renderPass, *stage.framebuffers, vk.swapChainExtent, *item.scene);
				}

				if (p == &m_PipelineParticles && item.pipelineIndex == 1) p->Record(imageIndex, stage.renderPass, *stage.framebuffers, vk.swapChainExtent, *item.scene);
			}
		}

		if (std::find(stage.pipelines.begin(), stage.pipelines.end(), &m_PipelinePostProcess) != stage.pipelines.end()) {
			static MeshScene dummy;
			m_PipelinePostProcess.Record(imageIndex, stage.renderPass, *stage.framebuffers, vk.swapChainExtent, dummy);
		}

		vkCmdEndRenderPass(vk.commandBuffers[frameIndex].m_VkCommandBuffer);
	}

	vk.commandBuffers[frameIndex].endRecording();

	// 6) Submit: RESET fence NOW (right before the one real submit)
	(vkResetFences(vk.device, 1, &inFlightFences[frameIndex]));

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore waitSems[] = { imageAvailableSemaphores[frameIndex] };
	VkSemaphore signalSems[] = { renderFinishedPerImage[imageIndex] };
	VkCommandBuffer cb = vk.commandBuffers[frameIndex].m_VkCommandBuffer;

	VkSubmitInfo submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = waitSems;
	submit.pWaitDstStageMask = waitStages;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &cb;
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = signalSems;

	VkResult sr = vkQueueSubmit(vk.graphicsQueue, 1, &submit, inFlightFences[frameIndex]);
	if (sr != VK_SUCCESS) {
		fprintf(stderr, "vkQueueSubmit failed: %s\n", VkResStr(sr));
		// Fence is currently UNSIGNALED (we just reset it). To avoid a hang on next frame’s wait,
		// legally signal it via zero-submit:
		(vkQueueSubmit(vk.graphicsQueue, 0, nullptr, inFlightFences[frameIndex]));
		return;
	}

	// 7) Present: wait on per-image present semaphore
	VkPresentInfoKHR present{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	present.waitSemaphoreCount = 1;
	present.pWaitSemaphores = signalSems;
	present.swapchainCount = 1;
	present.pSwapchains = &swapChain;
	present.pImageIndices = &imageIndex;

	VkResult pr = vkQueuePresentKHR(presentQueue, &present);
	if (pr == VK_ERROR_OUT_OF_DATE_KHR || pr == VK_SUBOPTIMAL_KHR) {
		// recreateSwapchain();
		return;
	}
	if (pr != VK_SUCCESS) {
		fprintf(stderr, "vkQueuePresentKHR failed: %s\n", VkResStr(pr));
		return;
	}

	vk.currentFrame++;
}

void RendererManager::Cleanup() {

}

void RendererManager::createInstance()
{

	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;

		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}

}
bool RendererManager::checkValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}
std::vector<const char*> RendererManager::getRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}
void RendererManager::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}
void RendererManager::setupDebugMessenger()
{
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}
void RendererManager::createSurface()
{
#ifdef _WIN32
	VulkanSurface_Windows surfaceCreator;
	surface = surfaceCreator.createSurface(instance, WindowManager::GetInstance().getPlatformWindow());
#else
#error "No Vulkan surface implementation for this OS"
#endif
}
void RendererManager::pickPhysicalDevice()
{
	auto& vulkan_vars = vulkanVars::GetInstance();


	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0)
		throw std::runtime_error("failed to find GPUs with Vulkan support!");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (auto& device : devices) {
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(device, &props);
		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
			&& isDeviceSuitable(device))
		{
			vulkan_vars.physicalDevice = device;
			std::cout << "Using discrete GPU: " << props.deviceName << "\n";
			break;
		}
	}

	if (vulkan_vars.physicalDevice == VK_NULL_HANDLE) {
		for (auto& device : devices) {
			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(device, &props);
			if (isDeviceSuitable(device)) {
				vulkan_vars.physicalDevice = device;
				std::cout << "Using non–discrete GPU: " << props.deviceName << "\n";
				break;
			}
		}
	}

	if (vulkan_vars.physicalDevice == VK_NULL_HANDLE)
		throw std::runtime_error("failed to find a suitable GPU!");
}
bool RendererManager::isDeviceSuitable(VkPhysicalDevice device) {
	QueueFamilyIndices indices = findQueueFamilies(device);
	bool extensionsSupported = checkDeviceExtensionSupport(device);
	return indices.isComplete() && extensionsSupported;

}
bool RendererManager::checkDeviceExtensionSupport(VkPhysicalDevice device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}
void RendererManager::createLogicalDevice()
{
	auto& vulkan_vars = vulkanVars::GetInstance();

	QueueFamilyIndices indices = findQueueFamilies(vulkan_vars.physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
	queueCreateInfo.queueCount = 1;

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(vulkan_vars.physicalDevice, &createInfo, nullptr, &vulkan_vars.device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(vulkan_vars.device, indices.graphicsFamily.value(), 0, &vulkan_vars.graphicsQueue);
	vkGetDeviceQueue(vulkan_vars.device, indices.presentFamily.value(), 0, &presentQueue);
}
void RendererManager::createSwapChain()
{
	auto& vulkan_vars = vulkanVars::GetInstance();
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(vulkan_vars.physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = findQueueFamilies(vulkan_vars.physicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(vulkan_vars.device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(vulkan_vars.device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(vulkan_vars.device, swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	vulkan_vars.swapChainExtent = extent;
}
SwapChainSupportDetails RendererManager::querySwapChainSupport(VkPhysicalDevice device) {
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}
VkSurfaceFormatKHR RendererManager::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}
VkPresentModeKHR RendererManager::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D RendererManager::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)()) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		WindowManager::GetInstance().getPlatformWindow()->getFramebufferSize(width, height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}
void RendererManager::createImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());
	auto& vulkan_vars = vulkanVars::GetInstance();
	for (size_t i = 0; i < swapChainImages.size(); i++) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(vulkan_vars.device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image views!");
		}
	}
}
void RendererManager::createRenderPasses() {
	auto& vk = vulkanVars::GetInstance();
	const VkFormat colorFormat = swapChainImageFormat;
	const VkFormat depthFormat = Pipeline::findDepthFormat(vk.physicalDevice, vk.device);

	// ---------- Offscreen (scene): color + depth, both transitioned to read-only at end ----------
	VkAttachmentDescription offColor{};
	offColor.format = colorFormat;
	offColor.samples = VK_SAMPLE_COUNT_1_BIT;
	offColor.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;      // clear every frame
	offColor.storeOp = VK_ATTACHMENT_STORE_OP_STORE;     // we sample this in post
	offColor.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	offColor.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	offColor.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	offColor.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentDescription offDepth{};
	offDepth.format = depthFormat;
	offDepth.samples = VK_SAMPLE_COUNT_1_BIT;
	offDepth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;      // clear every frame
	offDepth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;     // we sample this in post
	offDepth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	offDepth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	offDepth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	offDepth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

	VkAttachmentReference offColorRef{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference offDepthRef{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subOff{};
	subOff.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subOff.colorAttachmentCount = 1;
	subOff.pColorAttachments = &offColorRef;
	subOff.pDepthStencilAttachment = &offDepthRef;

	// Two external dependencies: ext->subpass (begin) and subpass->ext (end/visibility)
	VkSubpassDependency deps[2]{};

	// External -> subpass 0 (pass begin)
	deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	deps[0].dstSubpass = 0;
	deps[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	deps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	deps[0].srcAccessMask = 0;
	deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	deps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Subpass 0 -> External (pass end) : makes color/depth writes visible to next pass (shader read)
	deps[1].srcSubpass = 0;
	deps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	deps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
		VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	deps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	deps[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	deps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	deps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	std::array<VkAttachmentDescription, 2> offAtt{ offColor, offDepth };

	VkRenderPassCreateInfo rpOff{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	rpOff.attachmentCount = (uint32_t)offAtt.size();
	rpOff.pAttachments = offAtt.data();
	rpOff.subpassCount = 1;
	rpOff.pSubpasses = &subOff;
	rpOff.dependencyCount = 2;
	rpOff.pDependencies = deps;

	if (vkCreateRenderPass(vk.device, &rpOff, nullptr, &m_RenderPassOffscreen) != VK_SUCCESS)
		throw std::runtime_error("failed to create offscreen render pass");

	// ---------- Present (post) : color only, final -> PRESENT ----------
	VkAttachmentDescription presColor{};
	presColor.format = colorFormat;
	presColor.samples = VK_SAMPLE_COUNT_1_BIT;
	presColor.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;     // clear backbuffer
	presColor.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	presColor.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	presColor.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	presColor.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	presColor.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference presRef{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subPres{};
	subPres.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subPres.colorAttachmentCount = 1;
	subPres.pColorAttachments = &presRef;

	VkRenderPassCreateInfo rpPres{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	rpPres.attachmentCount = 1;
	rpPres.pAttachments = &presColor;
	rpPres.subpassCount = 1;
	rpPres.pSubpasses = &subPres;

	if (vkCreateRenderPass(vk.device, &rpPres, nullptr, &m_RenderPassPresent) != VK_SUCCESS)
		throw std::runtime_error("failed to create present render pass");

	// default renderPass used by 3D pipelines
	vk.renderPass = m_RenderPassOffscreen;
}
void RendererManager::initPipeLines() {
	auto& vulkan_vars = vulkanVars::GetInstance();

	// Normal pipelines (offscreen pass)
	m_Pipeline3d.Initialize("shaders/pbrShader.vert.spv", "shaders/pbrShader.frag.spv",
		Vertex::getBindingDescription(), Vertex::getAttributeDescriptions(),
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	PipelineConfig ncfg{};
	ncfg.renderPass = m_RenderPassOffscreen;
	ncfg.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	ncfg.enableDepthTest = true;
	ncfg.enableDepthWrite = false;
	ncfg.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; // or EQUAL
	m_PipelineNormals.Initialize("shaders/normals.vert.spv", "shaders/normals.frag.spv",
		Vertex::getBindingDescription(), Vertex::getAttributeDescriptions(), ncfg);

	m_PipelineParticles.Initialize("shaders/particleShader.vert.spv", "shaders/particleShader.frag.spv",
		Particle::getBindingDescription(), Particle::getAttributeDescriptions(),
		VK_PRIMITIVE_TOPOLOGY_POINT_LIST);

	// Post pipeline (present pass)
	PipelineConfig postCfg{};
	postCfg.renderPass = m_RenderPassPresent;
	postCfg.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	postCfg.enableDepthTest = false;
	postCfg.enableDepthWrite = false;
	postCfg.useVertexInput = false;   // fullscreen tri
	postCfg.usePushConstants = false;
	postCfg.fullscreenTriangle = true;
	postCfg.externalSetLayout = m_PostSetLayout;
	postCfg.externalSets = &m_PostDescSets;

	m_PipelinePostProcess.Initialize("shaders/postProcess.vert.spv",
		"shaders/postProcess.frag.spv",
		Vertex::getBindingDescription(), // ignored
		Vertex::getAttributeDescriptions(), // ignored
		postCfg);
}

void RendererManager::createFrameBuffers()
{
	auto& vulkan_vars = vulkanVars::GetInstance();
	swapChainFramebuffers.resize(swapChainImageViews.size());
	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		std::array<VkImageView, 2> attachments = {
			swapChainImageViews[i],
			m_Pipeline3d.getDepthImageView()
		}; 

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = vulkan_vars.renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = vulkan_vars.swapChainExtent.width;
		framebufferInfo.height = vulkan_vars.swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(vulkan_vars.device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}
void RendererManager::createSyncObjects(size_t /*currentFrame*/) {
	auto& vkvars = vulkanVars::GetInstance();

	imagesInFlight.assign(swapChainImages.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo     fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	// Per-frame acquire semaphores + per-frame fences
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		(vkCreateSemaphore(vkvars.device, &semInfo, nullptr, &imageAvailableSemaphores[i]));
		(vkCreateFence(vkvars.device, &fenceInfo, nullptr, &inFlightFences[i]));
	}

	// Per-image present semaphores
	renderFinishedPerImage.resize(swapChainImages.size());
	for (size_t i = 0; i < renderFinishedPerImage.size(); ++i) {
		(vkCreateSemaphore(vkvars.device, &semInfo, nullptr, &renderFinishedPerImage[i]));
	}
}

QueueFamilyIndices RendererManager::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

VkResult RendererManager::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void RendererManager::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void RendererManager::setupStages() {
	RenderStage gbufferStage;
	gbufferStage.name = "GBuffer";
	gbufferStage.renderPass = m_RenderPassOffscreen;
	gbufferStage.framebuffers = &m_OffscreenFramebuffers;
	gbufferStage.pipelines = { &m_Pipeline3d, &m_PipelineParticles, &m_PipelineNormals };
	gbufferStage.hasDepth = true;

	RenderStage postStage;
	postStage.name = "Post";
	postStage.renderPass = m_RenderPassPresent;
	postStage.framebuffers = &swapChainFramebuffers;
	postStage.pipelines = { &m_PipelinePostProcess };
	postStage.hasDepth = false;

	m_RenderStages = { gbufferStage, postStage };
}

void RendererManager::createOffscreenTargets() {
	auto& vk = vulkanVars::GetInstance();
	m_OffscreenTargets.resize(swapChainImages.size());
	for (size_t i = 0; i < m_OffscreenTargets.size(); ++i) {
		createImage2D(vk.physicalDevice, vk.device,
			vk.swapChainExtent.width, vk.swapChainExtent.height,
			swapChainImageFormat,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			m_OffscreenTargets[i].image, m_OffscreenTargets[i].memory);
		m_OffscreenTargets[i].view = createView(vk.device, m_OffscreenTargets[i].image,
			swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

void RendererManager::createFramebuffersOffscreen() {
	auto& vk = vulkanVars::GetInstance();
	m_OffscreenFramebuffers.resize(m_OffscreenTargets.size());
	for (size_t i = 0; i < m_OffscreenTargets.size(); ++i) {
		std::array<VkImageView, 2> atts = { m_OffscreenTargets[i].view,  m_OffscreenDepthTargets[i].view };
		VkFramebufferCreateInfo fci{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		fci.renderPass = m_RenderPassOffscreen; fci.attachmentCount = (uint32_t)atts.size();
		fci.pAttachments = atts.data(); fci.width = vk.swapChainExtent.width; fci.height = vk.swapChainExtent.height; fci.layers = 1;
		if (vkCreateFramebuffer(vk.device, &fci, nullptr, &m_OffscreenFramebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create offscreen framebuffer");
	}
}

void RendererManager::createFramebuffersPresent() {
	auto& vk = vulkanVars::GetInstance();
	swapChainFramebuffers.resize(swapChainImageViews.size());
	for (size_t i = 0; i < swapChainImageViews.size(); ++i) {
		VkImageView att = swapChainImageViews[i];
		VkFramebufferCreateInfo fci{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		fci.renderPass = m_RenderPassPresent; fci.attachmentCount = 1; fci.pAttachments = &att;
		fci.width = vk.swapChainExtent.width; fci.height = vk.swapChainExtent.height; fci.layers = 1;
		if (vkCreateFramebuffer(vk.device, &fci, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create present framebuffer");
	}
}

void RendererManager::createPostDescriptors() {
	auto& vk = vulkanVars::GetInstance();

	// set=0: binding 0 -> color, binding 1 -> depth
	VkDescriptorSetLayoutBinding b0{};
	b0.binding = 0;
	b0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	b0.descriptorCount = 1;
	b0.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding b1{};
	b1.binding = 1;
	b1.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	b1.descriptorCount = 1;
	b1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings{ b0, b1 };

	VkDescriptorSetLayoutCreateInfo lci{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	lci.bindingCount = (uint32_t)bindings.size();
	lci.pBindings = bindings.data();
	if (vkCreateDescriptorSetLayout(vk.device, &lci, nullptr, &m_PostSetLayout) != VK_SUCCESS)
		throw std::runtime_error("post set layout failed");

	VkDescriptorPoolSize poolSize{
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		static_cast<uint32_t>(m_OffscreenTargets.size() * 2)
	};
	VkDescriptorPoolCreateInfo pci{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	pci.poolSizeCount = 1;
	pci.pPoolSizes = &poolSize;
	pci.maxSets = (uint32_t)m_OffscreenTargets.size();

	if (vkCreateDescriptorPool(vk.device, &pci, nullptr, &m_PostDescPool) != VK_SUCCESS)
		throw std::runtime_error("post desc pool failed");

	// color sampler
	VkSamplerCreateInfo sci{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	sci.magFilter = VK_FILTER_LINEAR; sci.minFilter = VK_FILTER_LINEAR;
	sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sci.addressModeU = sci.addressModeV = sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	if (vkCreateSampler(vk.device, &sci, nullptr, &m_PostSampler) != VK_SUCCESS)
		throw std::runtime_error("post sampler failed");

	// depth sampler (no compare)
	VkSamplerCreateInfo dsi{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	dsi.magFilter = VK_FILTER_NEAREST; dsi.minFilter = VK_FILTER_NEAREST;
	dsi.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	dsi.addressModeU = dsi.addressModeV = dsi.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	dsi.compareEnable = VK_FALSE;
	if (vkCreateSampler(vk.device, &dsi, nullptr, &m_PostDepthSampler) != VK_SUCCESS)
		throw std::runtime_error("post depth sampler failed");

	// allocate sets
	std::vector<VkDescriptorSetLayout> layouts(m_OffscreenTargets.size(), m_PostSetLayout);
	VkDescriptorSetAllocateInfo ai{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	ai.descriptorPool = m_PostDescPool;
	ai.descriptorSetCount = (uint32_t)layouts.size();
	ai.pSetLayouts = layouts.data();

	m_PostDescSets.resize(layouts.size());
	if (vkAllocateDescriptorSets(vk.device, &ai, m_PostDescSets.data()) != VK_SUCCESS)
		throw std::runtime_error("post desc alloc failed");

	// write each set
	for (size_t i = 0; i < m_PostDescSets.size(); ++i) {
		VkDescriptorImageInfo colorII{};
		colorII.sampler = m_PostSampler;
		colorII.imageView = m_OffscreenTargets[i].view;
		colorII.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkDescriptorImageInfo depthII{};
		depthII.sampler = m_PostDepthSampler;
		depthII.imageView = m_OffscreenDepthTargets[i].view;
		depthII.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet writes[2]{};
		writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet = m_PostDescSets[i];
		writes[0].dstBinding = 0;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].descriptorCount = 1;
		writes[0].pImageInfo = &colorII;

		writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].dstSet = m_PostDescSets[i];
		writes[1].dstBinding = 1;
		writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[1].descriptorCount = 1;
		writes[1].pImageInfo = &depthII;

		vkUpdateDescriptorSets(vk.device, 2, writes, 0, nullptr);
	}
}

void RendererManager::writePostDescriptors()
{
	auto& vk = vulkanVars::GetInstance();

	for (size_t i = 0; i < m_PostDescSets.size(); ++i) {
		VkDescriptorImageInfo colorII{
			m_PostSampler,
			m_OffscreenTargets[i].view,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		VkDescriptorImageInfo depthII{
			m_PostDepthSampler,
			m_OffscreenDepthTargets[i].view,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
		};

		VkWriteDescriptorSet w[2]{};
		w[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		w[0].dstSet = m_PostDescSets[i];
		w[0].dstBinding = 0;
		w[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		w[0].descriptorCount = 1;
		w[0].pImageInfo = &colorII;

		w[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		w[1].dstSet = m_PostDescSets[i];
		w[1].dstBinding = 1;
		w[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		w[1].descriptorCount = 1;
		w[1].pImageInfo = &depthII;

		vkUpdateDescriptorSets(vk.device, 2, w, 0, nullptr);
	}
}


void RendererManager::createOffscreenDepthTargets() {
	auto& vk = vulkanVars::GetInstance();
	m_OffscreenDepthTargets.resize(swapChainImages.size());

	VkFormat depthFormat = Pipeline::findDepthFormat(vk.physicalDevice, vk.device);
	for (size_t i = 0; i < m_OffscreenDepthTargets.size(); ++i) {
		createImage2D(
			vk.physicalDevice, vk.device,
			vk.swapChainExtent.width, vk.swapChainExtent.height,
			depthFormat,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			m_OffscreenDepthTargets[i].image,
			m_OffscreenDepthTargets[i].memory
		);
		m_OffscreenDepthTargets[i].view = createView(
			vk.device, m_OffscreenDepthTargets[i].image,
			depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT
		);
	}
}
