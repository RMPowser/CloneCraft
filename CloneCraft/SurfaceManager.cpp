#include "SurfaceManager.h"


SurfaceManager::SurfaceManager(InstanceManager& instance, WindowManager& windowManager) {
	instanceManagerPointer = &instance;
	windowManagerPointer = &windowManager;
}

SurfaceManager::~SurfaceManager() {
	vkDestroySurfaceKHR(instanceManagerPointer->GetInstance(), surface, nullptr);
}

void SurfaceManager::CreateSurface() {
	if (glfwCreateWindowSurface(instanceManagerPointer->GetInstance(), windowManagerPointer->GetWindow(), nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

VkSurfaceKHR& SurfaceManager::GetSurface() {
	return surface;
}