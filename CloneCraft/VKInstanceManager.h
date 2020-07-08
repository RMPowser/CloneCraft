#pragma once
#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"
#include <iostream>
#include <vector>

class VKInstanceManager {
public:
	VKInstanceManager();
	~VKInstanceManager();
	void CreateVKInstance();
	VkInstance GetInstance();
	bool isValidationLayersEnabled();

private:
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif
	VkInstance instance;
	const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
	bool checkValidationLayerSupport();
	std::vector<const char*> getRequiredExtensions();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	static VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
};

