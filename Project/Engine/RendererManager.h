#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "Engine/Scene.h"
#include "Engine/Camera.h"
#include "Pipeline.h"

struct RenderItem {
    Scene* scene;
    int pipelineIndex;  // For example: 0 = particles, 1 = 3D objects.
};

class RendererManager {
public:
    RendererManager();
    ~RendererManager();

    // Initialize all renderer resources (Vulkan instance, device, swap chain, etc.)
    void Initialize();

    // Render one frame using the provided scene and camera
    void RenderFrame(const std::vector<RenderItem>& renderItems, Camera& camera);

    // Cleanup all renderer resources
    void Cleanup();

private:
    Pipeline m_Pipeline3d;
    Pipeline m_PipelineParticles;

    // Private helper functions can be added here (e.g. createInstance, createSwapChain, etc.
    VkSurfaceKHR surface;
    VkInstance instance;
    VkQueue presentQueue;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    std::vector<VkImageView> swapChainImageViews;

    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    void createInstance();
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    void setupDebugMessenger();
    VkDebugUtilsMessengerEXT debugMessenger;

    void createSurface();

    void pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    void createLogicalDevice();

    void createSwapChain();
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void createImageViews();


    void createRenderPass();

    void initPipeLines();
    void createFrameBuffers();

    void createSyncObjects();
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);


    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }
};
