#pragma once
#include "vulkan/vulkan.h"
#include "DeviceManager.h"
#include <vector>

class MemoryManager {
public:
	MemoryManager(DeviceManager& deviceManager);
	~MemoryManager();
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkDeviceMemory& GetTextureImageMemory() { return textureImageMemory; }
	VkDeviceMemory& GetDepthImageMemory() { return depthImageMemory; }
	std::vector<VkDeviceMemory>& GetUniformBuffersMemory() { return uniformBuffersMemory; }
	VkDeviceMemory& GetVertexBufferMemory() { return vertexBufferMemory; }
	VkDeviceMemory& GetIndexBufferMemory() { return indexBufferMemory; }

private:
	VkDeviceMemory depthImageMemory;
	VkDeviceMemory textureImageMemory;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	VkDeviceMemory vertexBufferMemory;
	VkDeviceMemory indexBufferMemory;

	DeviceManager* deviceManagerPointer;
};