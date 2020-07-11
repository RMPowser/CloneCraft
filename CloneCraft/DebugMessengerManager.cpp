#include "DebugMessengerManager.h"


DebugMessengerManager::DebugMessengerManager(InstanceManager& instanceManager) {
	instanceManagerPointer = &instanceManager;
}

DebugMessengerManager::~DebugMessengerManager() {
	if (instanceManagerPointer->isValidationLayersEnabled()) {
		DestroyDebugUtilsMessengerEXT(instanceManagerPointer->GetInstance(), debugMessenger, nullptr);
	}
}

void DebugMessengerManager::SetupDebugMessenger() {
	if (!instanceManagerPointer->isValidationLayersEnabled()) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	GlobalHelpers::populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(instanceManagerPointer->GetInstance(), &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

VkResult DebugMessengerManager::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DebugMessengerManager::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}
