#pragma once
#include "vulkan/vulkan.h"
#include "InstanceManager.h"

class DebugMessengerManager {
public:
	DebugMessengerManager(InstanceManager& instanceManager);
	~DebugMessengerManager();
	void SetupDebugMessenger();

private:
	VkDebugUtilsMessengerEXT debugMessenger;

	InstanceManager* instanceManagerPointer;

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
};