#ifdef _WIN32
#include <GLFW/glfw3native.h>
#include "VulkanSurface_Windows.h"
#include "PlatformWindow_Windows.h"

#include <stdexcept>

VkSurfaceKHR VulkanSurface_Windows::createSurface(VkInstance instance, PlatformWindow* window) {
    auto* win = static_cast<PlatformWindow_Windows*>(window);
    GLFWwindow* glfwWin = win->getGLFWwindow();
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance, glfwWin, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    return surface;
}
#endif
