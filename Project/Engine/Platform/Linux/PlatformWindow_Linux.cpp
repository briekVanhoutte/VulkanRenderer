#ifdef __linux__

#include "PlatformWindow_Linux.h"
#include <Engine/WindowManager.h>
#include <stdexcept>

PlatformWindow_Linux::PlatformWindow_Linux(int width, int height, const char* title) {
    if (!glfwInit()) throw std::runtime_error("Failed to initialize GLFW");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_window) throw std::runtime_error("Failed to create GLFW window");

    // Set the WindowManager instance as the user pointer.
    glfwSetWindowUserPointer(m_window, this);

    // Set up GLFW callbacks using our static functions.
    glfwSetKeyCallback(m_window, WindowManager::keyCallback);
    glfwSetCursorPosCallback(m_window, WindowManager::cursorPosCallback);
    glfwSetMouseButtonCallback(m_window, WindowManager::mouseButtonCallback);
}

PlatformWindow_Linux::~PlatformWindow_Linux() {
    if (m_window) glfwDestroyWindow(m_window);
    glfwTerminate();
}

void* PlatformWindow_Linux::getNativeHandle() {
    // On Linux, you might want to return the X11 or Wayland native handle,
    // but in most cases, Vulkan+GLFW only needs the GLFWwindow pointer.
    // You could use glfwGetX11Window(m_window) if you need the raw X11 window.
    // For pure Vulkan/GLFW, just return the pointer for now.
    return static_cast<void*>(m_window);
}

bool PlatformWindow_Linux::shouldClose() const {
    return glfwWindowShouldClose(m_window);
}

void PlatformWindow_Linux::pollEvents() {
    glfwPollEvents();
}

#endif // __linux__
