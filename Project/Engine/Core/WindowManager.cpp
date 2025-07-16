#include "WindowManager.h"
#include <iostream>
#include <stdexcept>


#ifdef _WIN32
    #include <Engine/Platform/Windows/PlatformWindow_Windows.h>
#endif

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

WindowManager& WindowManager::GetInstance() {
    static WindowManager instance;
    return instance;
}

WindowManager::WindowManager() {}

WindowManager::~WindowManager() {
    delete m_platformWindow;
}

void WindowManager::initWindow() {
#ifdef _WIN32
    m_platformWindow = new PlatformWindow_Windows(WIDTH, HEIGHT, "Vulkan");
#else
    #error "No platform window implementation provided for this OS"
#endif

}


PlatformWindow* WindowManager::getPlatformWindow() const
{
    return m_platformWindow;
}

void WindowManager::handleKeyEvent(int key, int scancode, int action, int mods) {
    std::cout << "Key event: key = " << key << ", action = " << action << "\n";
}

void WindowManager::handleCursorPos(double xpos, double ypos) {
    std::cout << "Cursor position: (" << xpos << ", " << ypos << ")\n";
}

void WindowManager::handleMouseButton(int button, int action, int mods) {
    std::cout << "Mouse button event: button = " << button << ", action = " << action << "\n";
}

#ifdef _WIN32
void WindowManager::keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods) {
    WindowManager* wm = static_cast<WindowManager*>(glfwGetWindowUserPointer(win));
    if (wm)
        wm->handleKeyEvent(key, scancode, action, mods);
}

void WindowManager::cursorPosCallback(GLFWwindow* win, double xpos, double ypos) {
    WindowManager* wm = static_cast<WindowManager*>(glfwGetWindowUserPointer(win));
    if (wm)
        wm->handleCursorPos(xpos, ypos);
}

void WindowManager::mouseButtonCallback(GLFWwindow* win, int button, int action, int mods) {
    WindowManager* wm = static_cast<WindowManager*>(glfwGetWindowUserPointer(win));
    if (wm)
        wm->handleMouseButton(button, action, mods);
}
#endif