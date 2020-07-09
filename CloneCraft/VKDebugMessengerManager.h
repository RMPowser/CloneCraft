#pragma once
#include "VKInstanceManager.h"

class VKDebugMessengerManager {
public:
	VKDebugMessengerManager(VKInstanceManager& instanceManager);
	~VKDebugMessengerManager();
	void SetupDebugMessenger();

private:
	VkDebugUtilsMessengerEXT debugMessenger;
	VKInstanceManager* instanceManagerPointer = nullptr;
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
};