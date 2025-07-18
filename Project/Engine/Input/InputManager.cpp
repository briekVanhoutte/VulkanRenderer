#include "InputManager.h"
#include <algorithm>
#include <iostream>

InputManager& InputManager::GetInstance() {
    static InputManager instance;
    return instance;
}

InputManager::InputManager() : mouseX(0.0), mouseY(0.0), m_LastMousePos(0,0) {}

InputManager::~InputManager() {}

void InputManager::Initialize(GLFWwindow* window) {
    glfwSetWindowUserPointer(window, this);

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

}

void InputManager::HandleCameraInputs(Camera* camera, float deltaTime)
{
    const float moveSpeed = 10.f; // Units per second (tweak as needed)
    const float rotationSpeed = 0.005f; // Radians per pixel (tweak as needed)

    // Movement
    if (IsKeyDown(GLFW_KEY_W)) {
        camera->translateForward(-moveSpeed * deltaTime);
    }
    if (IsKeyDown(GLFW_KEY_S)) {
        camera->translateForward(moveSpeed * deltaTime);
    }
    if (IsKeyDown(GLFW_KEY_A)) {
        camera->translateRight(-moveSpeed * deltaTime);
    }
    if (IsKeyDown(GLFW_KEY_D)) {
        camera->translateRight(moveSpeed * deltaTime);
    }

    // Rotation
    if (IsMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT)) {
        glm::vec2 currentPos(static_cast<float>(GetMouseX()), static_cast<float>(GetMouseY()));
        glm::vec2 offset = currentPos - m_LastMousePos;

        // Rotation should not depend on deltaTime (if using pixels for offset),
        // unless you want mouse movement to be time-dependent (which is rare).
        offset *= rotationSpeed;

        if (offset.x != 0.f || offset.y != 0.f) {
            camera->rotate(offset);
        }
        m_LastMousePos = currentPos;
    }
    else {
        m_LastMousePos = glm::vec2(static_cast<float>(GetMouseX()), static_cast<float>(GetMouseY()));
    }
}

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
