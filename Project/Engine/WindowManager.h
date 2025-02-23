#pragma once
#include <GLFW/glfw3.h>

class WindowManager {
public:
    // Get the singleton instance.
    static WindowManager& GetInstance();

    // Initializes GLFW and creates the window.
    void initWindow();

    // Returns the stored GLFW window pointer.
    GLFWwindow* getWindow() const;

    // Callback handling functions (to be implemented as needed).
    void handleKeyEvent(int key, int scancode, int action, int mods);
    void handleCursorPos(double xpos, double ypos);
    void handleMouseButton(int button, int action, int mods);

private:
    // Private constructor/destructor to enforce singleton.
    WindowManager();
    ~WindowManager();

    // Delete copy/move constructors and assignment operators.
    WindowManager(const WindowManager&) = delete;
    WindowManager& operator=(const WindowManager&) = delete;
    WindowManager(WindowManager&&) = delete;
    WindowManager& operator=(WindowManager&&) = delete;

    // The GLFW window pointer.
    GLFWwindow* window;

    // Static callback wrappers that retrieve the singleton instance.
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
};

