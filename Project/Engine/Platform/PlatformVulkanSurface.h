#pragma once

#include "vulkan\vulkan_core.h"
#include <Engine/Platform/PlatformWindow.h>

class PlatformVulkanSurface {
public:
    virtual ~PlatformVulkanSurface() {}
    virtual VkSurfaceKHR createSurface(VkInstance instance, PlatformWindow* window) = 0;
};
