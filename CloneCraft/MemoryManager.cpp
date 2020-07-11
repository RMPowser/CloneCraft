#include "MemoryManager.h"

MemoryManager::MemoryManager(DeviceManager& deviceManager) {
	deviceManagerPointer = &deviceManager;
}

MemoryManager::~MemoryManager() {
	vkFreeMemory(deviceManagerPointer->GetLogicalDevice(), textureImageMemory, nullptr);
	vkFreeMemory(deviceManagerPointer->GetLogicalDevice(), indexBufferMemory, nullptr);
	vkFreeMemory(deviceManagerPointer->GetLogicalDevice(), vertexBufferMemory, nullptr);
}

uint32_t MemoryManager::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(deviceManagerPointer->GetPhysicalDevice(), &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}