#include "InputManager.h"
#include <algorithm>
#include <iostream>

InputManager& InputManager::GetInstance() {
    static InputManager instance;
    return instance;
}

InputManager::InputManager() : mouseX(0.0), mouseY(0.0) {}

InputManager::~InputManager() {}

void InputManager::Initialize(GLFWwindow* window) {
    // Set this singleton as the user pointer for the window.
    glfwSetWindowUserPointer(window, this);

    // Install GLFW callbacks.
    glfwSetKeyCallback(window, InputManager::KeyCallback);
    glfwSetMouseButtonCallback(window, InputManager::MouseButtonCallback);
    glfwSetCursorPosCallback(window, InputManager::CursorPosCallback);
}

bool InputManager::IsKeyDown(int key) const {
    return std::find(keysDown.begin(), keysDown.end(), key) != keysDown.end();
}

bool InputManager::IsMouseButtonDown(int button) const {
    return std::find(mouseButtonsDown.begin(), mouseButtonsDown.end(), button) != mouseButtonsDown.end();
}

double InputManager::GetMouseX() const {
    return mouseX;
}

double InputManager::GetMouseY() const {
    return mouseY;
}

void InputManager::Update() {
    // For example, clear transient events or update state as needed.
    // Currently, nothing is required.
}

// Static callbacks.
void InputManager::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    InputManager* input = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (!input)
        return;

    if (action == GLFW_PRESS) {
        if (std::find(input->keysDown.begin(), input->keysDown.end(), key) == input->keysDown.end())
            input->keysDown.push_back(key);
    }
    else if (action == GLFW_RELEASE) {
        input->keysDown.erase(std::remove(input->keysDown.begin(), input->keysDown.end(), key), input->keysDown.end());
    }
}

void InputManager::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    InputManager* input = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (!input)
        return;

    if (action == GLFW_PRESS) {
        if (std::find(input->mouseButtonsDown.begin(), input->mouseButtonsDown.end(), button) == input->mouseButtonsDown.end())
            input->mouseButtonsDown.push_back(button);
    }
    else if (action == GLFW_RELEASE) {
        input->mouseButtonsDown.erase(std::remove(input->mouseButtonsDown.begin(), input->mouseButtonsDown.end(), button), input->mouseButtonsDown.end());
    }
}

void InputManager::CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    InputManager* input = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (!input)
        return;

    input->mouseX = xpos;
    input->mouseY = ypos;
}
