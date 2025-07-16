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

    glfwSetWindowUserPointer(m_window, this);

    glfwSetKeyCallback(m_window, WindowManager::keyCallback);
    glfwSetCursorPosCallback(m_window, WindowManager::cursorPosCallback);
    glfwSetMouseButtonCallback(m_window, WindowManager::mouseButtonCallback);
}

PlatformWindow_Linux::~PlatformWindow_Linux() {
    if (m_window) glfwDestroyWindow(m_window);
    glfwTerminate();
}

void* PlatformWindow_Linux::getNativeHandle() {
    return static_cast<void*>(m_window);
}

bool PlatformWindow_Linux::shouldClose() const {
    return glfwWindowShouldClose(m_window);
}

void PlatformWindow_Linux::pollEvents() {
    glfwPollEvents();
}

#endif 
