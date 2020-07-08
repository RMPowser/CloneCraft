#include "VKDebugMessengerManager.h"


VKDebugMessengerManager::VKDebugMessengerManager(VKInstanceManager& instanceManager) {
	instanceManagerPointer = &instanceManager;
}

VKDebugMessengerManager::~VKDebugMessengerManager() {
	if (instanceManagerPointer->isValidationLayersEnabled()) {
		DestroyDebugUtilsMessengerEXT(instanceManagerPointer->GetInstance(), debugMessenger, nullptr);
	}
}

void VKDebugMessengerManager::SetupDebugMessenger() {
	if (!instanceManagerPointer->isValidationLayersEnabled()) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	HelperFunctions::populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(instanceManagerPointer->GetInstance(), &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

VkResult VKDebugMessengerManager::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VKDebugMessengerManager::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}
