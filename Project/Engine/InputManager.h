#pragma once
#include <GLFW/glfw3.h>
#include <vector>
#include "Camera.h"

class InputManager {
public:
    static InputManager& GetInstance();

    // Initialize the InputManager by installing callbacks on the provided window.
    void Initialize(GLFWwindow* window);

    // Query functions.
    bool IsKeyDown(int key) const;
    bool IsMouseButtonDown(int button) const;
    double GetMouseX() const;
    double GetMouseY() const;

    // Update function (if you need per-frame processing)
    void Update();

    void HandleCameraInputs(Camera* camera, float deltaTime);
private:
    InputManager();
    ~InputManager();

    // Delete copy/move.
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;
    InputManager(InputManager&&) = delete;
    InputManager& operator=(InputManager&&) = delete;

    // Internal state.
    std::vector<int> keysDown;
    std::vector<int> mouseButtonsDown;
    double mouseX;
    double mouseY;

    glm::vec2 m_LastMousePos;

    // Static callback wrappers.
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
};
