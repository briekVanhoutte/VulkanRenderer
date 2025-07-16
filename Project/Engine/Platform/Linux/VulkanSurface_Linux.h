#pragma once
#ifdef __linux__

#include "../PlatformVulkanSurface.h"

class VulkanSurface_Linux : public PlatformVulkanSurface {
public:
    VkSurfaceKHR createSurface(VkInstance instance, PlatformWindow* window) override;
};

#endif 
