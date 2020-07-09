#include "VKSurfaceManager.h"


VKSurfaceManager::VKSurfaceManager(VKInstanceManager& instance) {
	instanceManagerPointer = &instance;
}

VKSurfaceManager::~VKSurfaceManager() {
	vkDestroySurfaceKHR(instanceManagerPointer->GetInstance(), surface, nullptr);
}

void VKSurfaceManager::createSurface(VKInstanceManager& instanceManager, WindowManager& windowManager) {
	if (glfwCreateWindowSurface(instanceManager.GetInstance(), windowManager.GetWindow(), nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

VkSurfaceKHR& VKSurfaceManager::GetSurface() {
	return surface;
}