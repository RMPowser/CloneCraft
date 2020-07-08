#include "VKInstanceManager.h"


VKInstanceManager::VKInstanceManager() {
}

VKInstanceManager::~VKInstanceManager() {
	vkDestroyInstance(instance, nullptr);
}

void VKInstanceManager::CreateVKInstance() {
	try {
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!\n");
		}
	} catch (std::runtime_error& e) {
		std::cerr << e.what();
		std::terminate();
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "CloneCraft";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "CloneCraft Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	auto extensions = getRequiredExtensions();

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		HelperFunctions::populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	} else {
		createInfo.enabledLayerCount = 0;

		createInfo.pNext = nullptr;
	}

	try {
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create vulkan instance\n");
		}
	} catch (std::runtime_error& e) {
		std::cerr << e.what();
		std::terminate();
	}
}

VkInstance VKInstanceManager::GetInstance() {
	return instance;
}

bool VKInstanceManager::isValidationLayersEnabled() {
	return enableValidationLayers;
}

bool VKInstanceManager::checkValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		std::cout << "searching for: " << layerName << "\n";
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			std::cout << "\t" << layerProperties.layerName << "\n";
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				std::cout << "\t" << "layer found :D" << "\n";
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

std::vector<const char*> VKInstanceManager::getRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	std::cout << "available extensions:\n";
	for (auto extension : extensions) {
		std::cout << "\t" << extension << std::endl;
	}
	return extensions;
}