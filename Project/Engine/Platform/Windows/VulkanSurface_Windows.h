// VulkanSurface_Windows.h
#pragma once
#ifdef _WIN32

#include "../PlatformVulkanSurface.h"
class VulkanSurface_Windows : public PlatformVulkanSurface {
public:
    VkSurfaceKHR createSurface(VkInstance instance, PlatformWindow* window) override;
};
#endif
