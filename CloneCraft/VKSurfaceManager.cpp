#include "VKSurfaceManager.h"


VKSurfaceManager::VKSurfaceManager(VKInstanceManager& instance, WindowManager& windowManager) {
	instanceManagerPointer = &instance;
	windowManagerPointer = &windowManager;
}

VKSurfaceManager::~VKSurfaceManager() {
	vkDestroySurfaceKHR(instanceManagerPointer->GetInstance(), surface, nullptr);
}

void VKSurfaceManager::createSurface() {
	if (glfwCreateWindowSurface(instanceManagerPointer->GetInstance(), windowManagerPointer->GetWindow(), nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

VkSurfaceKHR& VKSurfaceManager::GetSurface() {
	return surface;
}