#pragma once
#include "vulkan/vulkan.h"
#include "VKInstanceManager.h"
#include "WindowManager.h"

class VKSurfaceManager {
public:
	VKSurfaceManager(VKInstanceManager& instance, WindowManager& windowManager);
	~VKSurfaceManager();
	void CreateSurface();
	VkSurfaceKHR& GetSurface();

private:
	VkSurfaceKHR surface;
	WindowManager* windowManagerPointer;
	VKInstanceManager* instanceManagerPointer;
};