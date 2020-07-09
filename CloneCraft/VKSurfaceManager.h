#pragma once
#include "vulkan/vulkan.h"
#include "VKInstanceManager.h"
#include "WindowManager.h"

class VKSurfaceManager {
public:
	VKSurfaceManager(VKInstanceManager& instance);
	~VKSurfaceManager();
	void createSurface(VKInstanceManager& instanceManager, WindowManager& windowManager);
	VkSurfaceKHR& GetSurface();

private:
	VkSurfaceKHR surface;
	VKInstanceManager* instanceManagerPointer;
};