#pragma once
#include "VKInstanceManager.h"
#include "VKSurfaceManager.h"
#include <set>

class VKDeviceManager {
public:
	VKDeviceManager(VKInstanceManager& instanceManager, VKSurfaceManager& surfaceManager, const std::vector<const char*> deviceExtensions, const std::vector<const char*> requiredValidationLayers);
	~VKDeviceManager();
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	VkDevice& GetLogicalDevice();
	VkPhysicalDevice& GetPhysicalDevice();
	VkQueue& GetGraphicsQueue();
	VkQueue& GetPresentQueue();
	VkDeviceMemory& GetDeviceMemory();
	bool isDeviceSuitable(VkPhysicalDevice device);

private:
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice logicalDevice;
	VkDeviceMemory deviceMemory;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VKSurfaceManager* surfaceManagerPointer = nullptr;
	VKInstanceManager* instanceManagerPointer = nullptr;
	std::vector<const char*> deviceExtensions;
	std::vector<const char*> validationLayers;
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
};

