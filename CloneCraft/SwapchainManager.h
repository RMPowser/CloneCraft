#pragma once
#include "vulkan/vulkan.h"
#include "DeviceManager.h"
#include "SurfaceManager.h"
#include "WindowManager.h"
#include "TextureManager.h"
#include "MemoryManager.h"
#include "PipelineManager.h"
#include "BufferManager.h"
#include "RenderManager.h"
#include "DescriptorManager.h"
#include "GlobalHelpers.h"
#include <vector>

class SwapchainManager {
public:
	SwapchainManager(
		DeviceManager& deviceManager,
		SurfaceManager& surfaceManager,
		WindowManager& windowManager,
		TextureManager& textureManager,
		MemoryManager& memoryManager,
		PipelineManager& pipelineManager,
		BufferManager& bufferManager,
		RenderManager& renderManager,
		DescriptorManager& descriptorManager);
	~SwapchainManager();
	void createSwapChain();
	void cleanupSwapChain();
	void recreateSwapChain();

	VkSwapchainKHR&		GetSwapChain()			{ return swapchain; }
	VkExtent2D&			GetSwapchainExtent()	{ return swapchainExtent; }
	

private:
	VkSwapchainKHR swapchain;
	VkExtent2D swapchainExtent;
	
	DeviceManager* deviceManagerPointer;
	SurfaceManager* surfaceManagerPointer;
	WindowManager* windowManagerPointer;
	TextureManager* textureManagerPointer;
	MemoryManager* memoryManagerPointer;
	PipelineManager* pipelineManagerPointer;
	BufferManager* bufferManagerPointer;
	RenderManager* renderManagerPointer;
	DescriptorManager* descriptorManagerPointer;

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
};