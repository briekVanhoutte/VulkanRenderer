#ifdef __linux__

#include <GLFW/glfw3.h>
#include "VulkanSurface_Linux.h"
#include "PlatformWindow_Linux.h"
#include <stdexcept>

VkSurfaceKHR VulkanSurface_Linux::createSurface(VkInstance instance, PlatformWindow* window) {
    auto* win = static_cast<PlatformWindow_Linux*>(window);
    GLFWwindow* glfwWin = win->getGLFWwindow();
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance, glfwWin, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    return surface;
}

#endif 
