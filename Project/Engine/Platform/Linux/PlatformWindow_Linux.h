#pragma once

#ifdef __linux__

#include <GLFW/glfw3.h>
#include <Engine/Platform/PlatformWindow.h>

class PlatformWindow_Linux : public PlatformWindow {
public:
    PlatformWindow_Linux(int width, int height, const char* title);
    ~PlatformWindow_Linux();

    void getFramebufferSize(int& width, int& height) const override {
        glfwGetFramebufferSize(m_window, &width, &height);
    }
    void* getNativeHandle() override;
    GLFWwindow* getGLFWwindow() const { return m_window; }

    bool shouldClose() const override;
    void pollEvents() override;

private:
    GLFWwindow* m_window;
};

#endif 
