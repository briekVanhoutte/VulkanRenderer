#ifdef _WIN32

#include "PlatformWindow_Windows.h"
#include <Engine/Core/WindowManager.h>

#define GLFW_EXPOSE_NATIVE_WIN32

#ifdef GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

PlatformWindow_Windows::PlatformWindow_Windows(int width, int height, const char* title) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);

    // Set the WindowManager instance as the user pointer.
    glfwSetWindowUserPointer(m_window, this);

    // Set up GLFW callbacks using our static functions.
    glfwSetKeyCallback(m_window, WindowManager::keyCallback);
    glfwSetCursorPosCallback(m_window, WindowManager::cursorPosCallback);
    glfwSetMouseButtonCallback(m_window, WindowManager::mouseButtonCallback);
}

PlatformWindow_Windows::~PlatformWindow_Windows() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void* PlatformWindow_Windows::getNativeHandle() {
    // On Windows, this is the HWND, retrievable via glfwGetWin32Window.

    return glfwGetWin32Window(m_window);
}

bool PlatformWindow_Windows::shouldClose() const
{
     return glfwWindowShouldClose(m_window); 
}

void PlatformWindow_Windows::pollEvents() {
    glfwPollEvents();
}

#endif // _WIN32
