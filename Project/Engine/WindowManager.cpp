#include "WindowManager.h"
#include <iostream>
#include <stdexcept>

// You can define your window dimensions here.
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

WindowManager& WindowManager::GetInstance() {
    static WindowManager instance;
    return instance;
}

WindowManager::WindowManager() : window(nullptr) {}

WindowManager::~WindowManager() {
    if (window)
        glfwDestroyWindow(window);
    glfwTerminate();
}

void WindowManager::initWindow() {
    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW!");

    // Tell GLFW not to create an OpenGL context.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    if (!window)
        throw std::runtime_error("Failed to create GLFW window!");

    // Set the WindowManager instance as the user pointer.
    glfwSetWindowUserPointer(window, this);

    // Set up GLFW callbacks using our static functions.
    glfwSetKeyCallback(window, WindowManager::keyCallback);
    glfwSetCursorPosCallback(window, WindowManager::cursorPosCallback);
    glfwSetMouseButtonCallback(window, WindowManager::mouseButtonCallback);
}

GLFWwindow* WindowManager::getWindow() const {
    return window;
}

void WindowManager::handleKeyEvent(int key, int scancode, int action, int mods) {
    // Fill in your key handling logic here.
    std::cout << "Key event: key = " << key << ", action = " << action << "\n";
}

void WindowManager::handleCursorPos(double xpos, double ypos) {
    // Fill in your cursor movement logic here.
    std::cout << "Cursor position: (" << xpos << ", " << ypos << ")\n";
}

void WindowManager::handleMouseButton(int button, int action, int mods) {
    // Fill in your mouse button handling logic here.
    std::cout << "Mouse button event: button = " << button << ", action = " << action << "\n";
}

// Static callback wrappers.

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
