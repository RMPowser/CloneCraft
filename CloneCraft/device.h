#pragma once
#include "INCLUDER.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// pick a suitable device with vulkan support
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void pickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("failed to fund GPU with Vulkan support");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& device : devices) {
		if (isDeviceSuitable(device)) {
			physicalDevice = device;
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find suitable GPU");
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// create a logical device to interface with the physical device
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void createLogicalDevice() {
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

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

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// make sure the queuefamilies actually exist, the required extensions are supported, and the swap chain has at least one
// supported image format and one supported presentation mode given the window surface we have
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool isDeviceSuitable(VkPhysicalDevice device) {
	QueueFamilyIndices indices = findQueueFamilies(device);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// enumerate extensions and check if all required extentions are among them
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
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

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	// First we need to query info about the available types of memory using vkGetPhysicalDeviceMemoryProperties.
	// The VkPhysicalDeviceMemoryProperties structure has two arrays memoryTypes and memoryHeaps. Memory heaps are distinct
	// memory resources like dedicated VRAM and swap space in RAM for when VRAM runs out. The different types of memory exist 
	// within these heaps. Right now we'll only concern ourselves with the type of memory and not the heap it comes from, but
	// you can imagine that this can affect performance.
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	// find a memory type that is suitable for the buffer itself.
	// The typeFilter parameter will be used to specify the bit field of memory types that are suitable. That means that we can
	// find the index of a suitable memory type by simply iterating over them and checking if the corresponding bit is set to 1.
	// but we also need to be able to write our vertex data to that memory. The memoryTypes array consists of VkMemoryType
	// structs that specify the heap and properties of each type of memory. The properties define special features of the
	// memory, like being able to map it so we can write to it from the CPU. This property is indicated with
	// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, but we also need to use the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT property. We'll see
	// why when we map the memory.
	// We may have more than one desirable property, so we should check if the result of the bitwise AND is not just non-zero, 
	// but equal to the desired properties bit field. If there is a memory type suitable for the buffer that also has all of 
	// the properties we need, then we return its index, otherwise we throw an exception.
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}