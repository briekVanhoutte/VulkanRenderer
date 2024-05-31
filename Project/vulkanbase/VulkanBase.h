#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "VulkanUtil.h"
#include "../Project/Engine/ShaderBase.h"
#include "../Project/Engine/CommandPool.h"
#include "../Project/Engine/Mesh.h"
#include "../Project/Engine/Pipeline.h"


#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <optional>
#include <set>
#include <limits>
#include <algorithm>
#include "engine/Scene.h"
#include "Engine/Camera.h"
#include <chrono>

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};



class VulkanBase {
public:
	void run() {
		initWindow();
	
		initVulkan();

		mainLoop();
		cleanup();
	}

private:
	CommandPool m_CommandPool{};
	Pipeline m_Pipeline3d;

	std::vector<int> keysDown{};
	std::vector<int> mouseDown{};

	Scene m_Scene = Scene{};
	Camera m_Camera = Camera{};

	void initCamera() {
		float fov{ 90.f };
		float aspectRatio{ float(WIDTH) / float(HEIGHT) };
		glm::vec3 cameraStartLocation{ 0.f,0.f,-40.f };

		m_Camera.Initialize(fov, cameraStartLocation, aspectRatio);
	}

	void initScene() {
		std::vector<Vertex> vertices{};
		std::vector< uint16_t> indices{};
		ParseOBJ("Resources/vehicle.obj", vertices, indices,{0.2f,0.6f,0.2f}, false);
		glm::vec3 pos{ 20.f,0.f,0.f };
		glm::vec3 scale{0.9f,0.9f,0.9f };
		glm::vec3 rot{ 0,0.f,0.f };

		if (vertices.size() > 0 && indices.size() > 0) {
			m_Scene.addModel(vertices, indices, pos, scale, rot);
		}
		else
		{
			std::cout << "object 1 did not load correctly" << std::endl;
		}
		

		std::vector<Vertex> vertices2{};
		std::vector< uint16_t> indices2{};
		ParseOBJ("Resources/tuktuk.obj", vertices2, indices2, { 0.6f,0.2f,0.2f }, false);

		glm::vec3 pos2{ -20.f,-10.f,0.f };
		glm::vec3 scale2{ 1.f,1.f,1.f };
		glm::vec3 rot2{ 0.f,90.f,0.f };

		if (vertices2.size() > 0 && indices2.size() > 0) {
			m_Scene.addModel(vertices2, indices2, pos2, scale2, rot2);
		}
		else
		{
			std::cout << "object 2 did not load correctly" << std::endl;
		}

		
	}

	void initPipeLine()
	{
		//initScene();
		initScene();
		m_Pipeline3d.SetScene(m_Scene);
		m_Pipeline3d.Initialize(physicalDevice,device,m_CommandPool, "shaders/shader3d.vert.spv", "shaders/shader3d.frag.spv",renderPass, graphicsQueue, swapChainExtent);
	}

	void initVulkan() {
		createInstance();
		setupDebugMessenger();
		createSurface();

		pickPhysicalDevice();
		createLogicalDevice();

		createSwapChain();
		createImageViews();

		createRenderPass();

		initCamera();

		m_CommandPool.initialize(device, findQueueFamilies(physicalDevice));

		initPipeLine();
		createFrameBuffers();

		createSyncObjects();
	}

	void mainLoop() {
		
		auto startTime = std::chrono::high_resolution_clock::now();
		float deltaTime = 0.f;

		while (!glfwWindowShouldClose(window)) {
			auto currentTime = std::chrono::high_resolution_clock::now();
			deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
			startTime = currentTime;

			glfwPollEvents();
			// week 06
			drawFrame3d();
			HandleKeyInputs(deltaTime);
			HandleMouseInputs(deltaTime);
			m_Camera.update();

		}
		vkDeviceWaitIdle(device);
	}

	void cleanup() {
		vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
		vkDestroyFence(device, inFlightFence, nullptr);

		vkDestroyCommandPool(device, m_CommandPool.m_CommandPool  , nullptr);
		for (auto framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}

		vkDestroyRenderPass(device, renderPass, nullptr);

		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}

		m_Scene.deleteScene(device);
		//m_Mesh.destroyMesh(device);
		m_Pipeline3d.Destroy(device);

		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}
		vkDestroySwapchainKHR(device, swapChain, nullptr);

		vkDestroyDevice(device, nullptr);

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);
		glfwTerminate();
	}

	
	void HandleKeyInputs(float deltaTime);
	void HandleMouseInputs(float deltaTime);
	

	void createSurface() {
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	GLFWwindow* window;
	void initWindow();

	void keyEvent(int key, int scancode, int action, int mods);

	void mouseMove(GLFWwindow* window, double xpos, double ypos);

	void mouseEvent(GLFWwindow* window, int button, int action, int mods);

	float m_Radius;
	float m_Rotation;
	glm::vec2 m_LastMousePos;

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkRenderPass renderPass;

	void createFrameBuffers();
	void createRenderPass();

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	std::vector<VkImageView> swapChainImageViews;

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	void createSwapChain();
	void createImageViews();

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	
	void pickPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice device);
	void createLogicalDevice();

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkDevice device = VK_NULL_HANDLE;
	VkSurfaceKHR surface;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkFence inFlightFence;

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void setupDebugMessenger();
	std::vector<const char*> getRequiredExtensions();
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	void createInstance();

	void createSyncObjects();
	void drawFrame3d();

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
		return VK_FALSE;
	}
};