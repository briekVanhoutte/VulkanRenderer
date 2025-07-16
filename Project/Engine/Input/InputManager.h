#pragma once
#include <GLFW/glfw3.h>
#include <vector>
#include <Engine/Math/Camera.h>

class InputManager {
public:
    static InputManager& GetInstance();

    void Initialize(GLFWwindow* window);

    bool IsKeyDown(int key) const;
    bool IsMouseButtonDown(int button) const;
    double GetMouseX() const;
    double GetMouseY() const;

    void Update();

    void HandleCameraInputs(Camera* camera, float deltaTime);
private:
    InputManager();
    ~InputManager();

    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;
    InputManager(InputManager&&) = delete;
    InputManager& operator=(InputManager&&) = delete;

    std::vector<int> keysDown;
    std::vector<int> mouseButtonsDown;
    double mouseX;
    double mouseY;

    glm::vec2 m_LastMousePos;

    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
};
