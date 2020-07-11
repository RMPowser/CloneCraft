#pragma once
#include "vulkan/vulkan.h"
#include "SurfaceManager.h"
#include "InstanceManager.h"
#include <set>

class DeviceManager {
public:
	DeviceManager(InstanceManager& instanceManager, SurfaceManager& surfaceManager, const std::vector<const char*> deviceExtensions, const std::vector<const char*> requiredValidationLayers);
	~DeviceManager();
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

	SurfaceManager* surfaceManagerPointer = nullptr;
	InstanceManager* instanceManagerPointer = nullptr;
	std::vector<const char*> deviceExtensions;
	std::vector<const char*> validationLayers;

	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
};

