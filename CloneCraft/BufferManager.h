#pragma once
#include "vulkan/vulkan.h"
#include "TextureManager.h"
#include "RenderManager.h"
#include "DescriptorManager.h"
#include <vector>
#include <chrono>

class PipelineManager;
class SwapchainManager;

class BufferManager {
public:
	BufferManager(
		TextureManager& textureManager,
		SwapchainManager& swapchainManager,
		DeviceManager& deviceManager,
		MemoryManager& memoryManager,
		PipelineManager& pipelineManager,
		RenderManager& renderManager,
		DescriptorManager& descriptorManager,
		SyncManager& syncManager);
	~BufferManager();
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void createFrameBuffers();
	void createUniformBuffers();
	void createCommandBuffers();
	void createVertexBuffer();
	void createIndexBuffer();
	void updateUniformBuffer(uint32_t currentImage);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	std::vector<VkFramebuffer>&		GetSwapchainFramebuffers()	{ return swapchainFramebuffers; }
	std::vector<VkBuffer>&			GetUniformBuffers()			{ return uniformBuffers; }
	std::vector<VkCommandBuffer>&	GetCommandBuffers()			{ return commandBuffers; }
	VkBuffer&						GetVertexBuffer()			{ return vertexBuffer; }
	VkBuffer&						GetIndexBuffer()			{ return indexBuffer; }


private:
	std::vector<VkFramebuffer> swapchainFramebuffers;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkCommandBuffer> commandBuffers;
	VkBuffer vertexBuffer;
	VkBuffer indexBuffer;

	TextureManager* textureManagerPointer;
	SwapchainManager* swapchainManagerPointer;
	DeviceManager* deviceManagerPointer;
	MemoryManager* memoryManagerPointer;
	PipelineManager* pipelineManagerPointer;
	RenderManager* renderManagerPointer;
	DescriptorManager* descriptorManagerPointer;
	SyncManager* syncManagerPointer;

};

