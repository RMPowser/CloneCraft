#include "VKDeviceManager.h"

VKDeviceManager::VKDeviceManager(VKInstanceManager& instanceManager, VKSurfaceManager& surfaceManager, std::vector<const char*> _deviceExtensions, std::vector<const char*> _validationLayers) {
	instanceManagerPointer = &instanceManager;
	surfaceManagerPointer = &surfaceManager;
	deviceExtensions = _deviceExtensions;
	validationLayers = _validationLayers;
}

VKDeviceManager::~VKDeviceManager() {
	vkDestroyDevice(logicalDevice, nullptr);
}

void VKDeviceManager::pickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instanceManagerPointer->GetInstance(), &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPU with Vulkan support");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instanceManagerPointer->GetInstance(), &deviceCount, devices.data());

	for (auto& device : devices) {
		if (isDeviceSuitable(device)) {
			physicalDevice = device;
			VkPhysicalDeviceProperties prop;
			vkGetPhysicalDeviceProperties(physicalDevice, &prop);
			std::cout << "physical device found: " << prop.deviceName << "\n";
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find suitable GPU");
	}
}

void VKDeviceManager::createLogicalDevice() {
	GlobalHelpers::QueueFamilyIndices indices = GlobalHelpers::findQueueFamilies(physicalDevice, surfaceManagerPointer->GetSurface());

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (instanceManagerPointer->GetInstance()) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);
}

VkDevice& VKDeviceManager::GetLogicalDevice() {
	return logicalDevice;
}

VkPhysicalDevice& VKDeviceManager::GetPhysicalDevice() {
	return physicalDevice;
}

VkQueue& VKDeviceManager::GetGraphicsQueue() {
	return graphicsQueue;
}

VkQueue& VKDeviceManager::GetPresentQueue() {
	return presentQueue;
}

bool VKDeviceManager::isDeviceSuitable(VkPhysicalDevice device) {
	GlobalHelpers::QueueFamilyIndices indices = GlobalHelpers::findQueueFamilies(device, surfaceManagerPointer->GetSurface());

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		GlobalHelpers::SwapChainSupportDetails swapChainSupport = GlobalHelpers::querySwapChainSupport(device, surfaceManagerPointer->GetSurface());
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

bool VKDeviceManager::checkDeviceExtensionSupport(VkPhysicalDevice device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

