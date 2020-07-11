#pragma once
#include "vulkan/vulkan.h"
#include "WindowManager.h"
#include "InstanceManager.h"

class SurfaceManager {
public:
	SurfaceManager(InstanceManager& instance, WindowManager& windowManager);
	~SurfaceManager();
	void CreateSurface();
	VkSurfaceKHR& GetSurface();

private:
	VkSurfaceKHR surface;

	WindowManager* windowManagerPointer;
	InstanceManager* instanceManagerPointer;
};