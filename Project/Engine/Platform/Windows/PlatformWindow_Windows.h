#pragma once

#ifdef _WIN32


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <GLFW/glfw3.h>
#include <Engine/Platform/PlatformWindow.h>

class PlatformWindow_Windows : public PlatformWindow {
public:
    PlatformWindow_Windows(int width, int height, const char* title);
    ~PlatformWindow_Windows();
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