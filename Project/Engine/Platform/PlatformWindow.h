#pragma once

class PlatformWindow {
public:
    virtual ~PlatformWindow() {}
    virtual void* getNativeHandle() = 0;
    virtual bool shouldClose() const = 0;
    virtual void pollEvents() = 0;
    virtual void getFramebufferSize(int& width, int& height) const = 0;
    // todo Add more as needed (resize, close, etc.)
};
