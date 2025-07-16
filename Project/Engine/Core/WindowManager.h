#pragma once
#include <GLFW/glfw3.h>
#include <Engine/Platform/PlatformWindow.h>

class WindowManager {
public:
    static WindowManager& GetInstance();

    void initWindow();

    PlatformWindow* getPlatformWindow() const;

    void handleKeyEvent(int key, int scancode, int action, int mods);
    void handleCursorPos(double xpos, double ypos);
    void handleMouseButton(int button, int action, int mods);

#ifdef _WIN32
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
#endif

private:
    WindowManager();
    ~WindowManager();

    WindowManager(const WindowManager&) = delete;
    WindowManager& operator=(const WindowManager&) = delete;
    WindowManager(WindowManager&&) = delete;
    WindowManager& operator=(WindowManager&&) = delete;

    PlatformWindow* m_platformWindow = nullptr;
};

