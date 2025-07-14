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

#include "../Project/Engine/PhysxBase.h"
#include "../Project/Engine/vulkanVars.h"
#include "../Project/Engine/MeshScene.h"
#include "../Project/Engine/ParticleScene.h"

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
#include <iomanip>
#include <PxPhysicsAPI.h>


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

		initPhysx();

		initVulkan();

		mainLoop();
		cleanup();
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

		auto& vulkan_vars = vulkanVars::GetInstance();
		vulkan_vars.commandPoolModelPipeline.initialize(findQueueFamilies(vulkan_vars.physicalDevice));
		vulkan_vars.commandPoolParticlesPipeline.initialize(findQueueFamilies(vulkan_vars.physicalDevice));
		//m_CommandPool.initialize(device, findQueueFamilies(physicalDevice));

		initPipeLine();
		createFrameBuffers();

		createSyncObjects();
	}

private:
	Pipeline m_Pipeline3d;
	Pipeline m_PipelineParticles;

	std::vector<int> keysDown{};
	std::vector<int> mouseDown{};

	MeshScene m_Scene = MeshScene{};
	ParticleScene m_Scene2 = ParticleScene{};

	Camera m_Camera = Camera{};

	void initCamera() {
		float fov{ 90.f };
		float aspectRatio{ float(WIDTH) / float(HEIGHT) };
		glm::vec3 cameraStartLocation{ -4.f,-3.f,-25.f };

		m_Camera.Initialize(fov, cameraStartLocation, aspectRatio);
	}

	void initPhysx() {
		auto& physx = PhysxBase::GetInstance();
		physx.initPhysics(false);
	};

	unsigned int m_MovingwallIndex;

	void initScene() {
		auto& vulkan_vars = vulkanVars::GetInstance();
		auto& physxBase = PhysxBase::GetInstance();

		// add models to pipeline
		/*std::vector<Vertex> vertices{};
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
		}*/


		glm::vec3 posParticles{ 0.f,-10.f,-10.f };
		glm::vec3 scaleParticles{ 1.f,1.f,1.f };
		glm::vec3 rotParticles{ 0.f,0.f,0.f };

		m_Scene2.addParticleGroup(physxBase.getParticleBuffer()->getPositionInvMasses(), physxBase.getParticleBuffer()->getNbActiveParticles(), physxBase.m_Particles, posParticles, scaleParticles, rotParticles);

		// create container for particles

		posParticles.y += 3.f;

		// back
		glm::vec3 posBackWall{ posParticles };
		posBackWall.z += 3.f;
		posBackWall.x -= 3.f;
		m_Scene.addRectangle({ 0.f, 0.f, -1.f }, { 0.f,0.9f,0.f }, 12.f, 6.f, posBackWall, scaleParticles, rotParticles);

		auto& physx_base = PhysxBase::GetInstance();

		// right
		glm::vec3 posLeftWall{ posParticles };
		posLeftWall.x += physx_base.getRightWallLocation();
		m_MovingwallIndex = m_Scene.addRectangle({ 1.f, 0.f, 0.f }, { 0.f,0.9f,0.f }, 6.f, 6.f, posLeftWall, scaleParticles, rotParticles);
		
		// left
		glm::vec3 posRightWall{ posParticles };
		posRightWall.x += 3.f;
		m_Scene.addRectangle({ -1.f, 0.f, 0.f }, { 0.f,0.9f,0.f }, 6.f, 6.f, posRightWall, scaleParticles, rotParticles);

		// bot
		glm::vec3 posBotWall{ posParticles };
		posBotWall.y -= 3.f;
		posBotWall.x -= 3.f;
		m_Scene.addRectangle({ 0.f, 1.f, 0.f }, { 0.f,0.9f,0.f }, 12.f, 6.f, posBotWall, scaleParticles, rotParticles);
		

		m_Scene.initObject(vulkan_vars.physicalDevice, vulkan_vars.device, vulkan_vars.commandPoolModelPipeline.m_CommandPool, vulkan_vars.graphicsQueue);

		
		
	}

	void initPipeLine()
	{
		
		//initScene();
		auto& vulkan_vars = vulkanVars::GetInstance();
		initScene();
		
		//vulkan_vars.commandBuffer = vulkan_vars.commandPoolModelPipeline.createCommandBuffer();

		m_Pipeline3d.Initialize( "shaders/shader3d.vert.spv", "shaders/shader3d.frag.spv", Vertex::getBindingDescription(),Vertex::getAttributeDescriptions() , VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		m_PipelineParticles.Initialize( "shaders/computeShader.vert.spv", "shaders/computeShader.frag.spv",  Particle::getBindingDescription(), Particle::getAttributeDescriptions() , VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
	}

	

	void mainLoop() {
		auto& vulkan_vars = vulkanVars::GetInstance();

		std::cout << "Controls: " << std::endl;
		std::cout << "movement: wasd " << std::endl;
		std::cout << "camera: lmb + mouse " << std::endl;
		std::cout << "move wall: arrow left + right " << std::endl;

		auto startTime = std::chrono::high_resolution_clock::now();
		float deltaTime = 0.f;
		float totalTime = 0.f;
		auto& physx = PhysxBase::GetInstance();
		int frameCount = 0;
		int fps = 50;
		while (!glfwWindowShouldClose(window)) {
			auto currentTime = std::chrono::high_resolution_clock::now();
			deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
			startTime = currentTime;

			glfwPollEvents();
			// week 06

			physx.stepPhysics(false);
			drawFrame3d();
			HandleKeyInputs(deltaTime /fps);
			HandleMouseInputs(deltaTime /fps);
			m_Camera.update();

			frameCount++;
			totalTime += deltaTime;
			if (totalTime >= 1.0f) {
				fps = frameCount;
				//std::cout << "FPS: " << std::fixed << std::setprecision(2) << fps << std::endl;
				frameCount = 0;
				startTime = std::chrono::high_resolution_clock::now(); // Reset start time
				totalTime = 0.f;
			}
		}
		vkDeviceWaitIdle(vulkan_vars.device);
	}

	void cleanup() {
		auto& vulkan_vars = vulkanVars::GetInstance();

		vkDestroySemaphore(vulkan_vars.device, renderFinishedSemaphore, nullptr);
		vkDestroySemaphore(vulkan_vars.device, imageAvailableSemaphore, nullptr);
		vkDestroyFence(vulkan_vars.device, inFlightFence, nullptr);

		vkDestroyCommandPool(vulkan_vars.device, vulkan_vars.commandPoolModelPipeline.m_CommandPool  , nullptr);
		vkDestroyCommandPool(vulkan_vars.device, vulkan_vars.commandPoolParticlesPipeline.m_CommandPool  , nullptr);
		for (auto framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(vulkan_vars.device, framebuffer, nullptr);
		}

		vkDestroyRenderPass(vulkan_vars.device, vulkan_vars.renderPass, nullptr);

		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(vulkan_vars.device, imageView, nullptr);
		}

		m_Scene.deleteScene(vulkan_vars.device);
		m_Scene2.deleteScene(vulkan_vars.device);
		//m_Mesh.destroyMesh(device);
		m_Pipeline3d.Destroy(vulkan_vars.device);
		m_PipelineParticles.Destroy(vulkan_vars.device);

		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}
		vkDestroySwapchainKHR(vulkan_vars.device, swapChain, nullptr);

		vkDestroyDevice(vulkan_vars.device, nullptr);

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

	void createFrameBuffers();
	void createRenderPass();

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;

	std::vector<VkImageView> swapChainImageViews;

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	void createSwapChain();
	void createImageViews();

	VkQueue presentQueue;
	
	void pickPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice device);
	void createLogicalDevice();

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
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