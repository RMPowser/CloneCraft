#pragma once
#include "vulkan/vulkan.h"
#include "DeviceManager.h"
#include "SwapchainManager.h"
#include "MemoryManager.h"
#include "BufferManager.h"
#include "SyncManager.h"
#include <vector>
#include <string>

class TextureManager {
public:
	TextureManager(DeviceManager& deviceManager, SwapchainManager& swapchainManager, MemoryManager& memoryManager, BufferManager& bufferManager, SyncManager& syncManager);
	~TextureManager();
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void createImageViews();
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void createTextureSampler();
	void createTextureImage(const std::string TEXTURE_PATH);
	void createTextureImageView();
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void createDepthResources();
	VkFormat findDepthFormat();
	
	std::vector<VkImage>&		GetSwapchainImages()		{ return swapchainImages; }
	VkFormat&					GetSwapchainImageFormat()	{ return swapchainImageFormat; }
	std::vector<VkImageView>&	GetSwapchainImageViews()	{ return swapchainImageViews; }
	VkImage&					GetDepthImage()				{ return depthImage; }
	VkImageView&				GetDepthImageView()			{ return depthImageView; }
	VkImage&					GetTextureImage()			{ return textureImage; }
	VkImageView&				GetTextureImageView()		{ return textureImageView; }
	VkSampler&					GetTextureSampler()			{ return textureSampler; }

private:
	VkImage depthImage;
	VkImageView depthImageView;
	VkSampler textureSampler;
	VkImage textureImage;
	VkImageView textureImageView;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	VkFormat swapchainImageFormat;

	DeviceManager* deviceManagerPointer;
	SwapchainManager* swapchainManagerPointer;
	MemoryManager* memoryManagerPointer;
	BufferManager* bufferManagerPointer;
	SyncManager* syncManagerPointer;

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
};

