#include "TextureManager.h"
#define STB_IMAGE_IMPLEMENTATION // The header only defines the prototypes of the functions by default. One code file needs to include the header with the STB_IMAGE_IMPLEMENTATION definition to include the function bodies, otherwise we'll get linking errors.
#include <stb_image.h>

TextureManager::TextureManager(DeviceManager& deviceManager, SwapchainManager& swapchainManager, MemoryManager& memoryManager, BufferManager& bufferManager, SyncManager& syncManager) {
	deviceManagerPointer = &deviceManager;
	swapchainManagerPointer = &swapchainManager;
	memoryManagerPointer = &memoryManager;
	bufferManagerPointer = &bufferManager;
	syncManagerPointer = &syncManager;
}

TextureManager::~TextureManager() {
	vkDestroySampler(deviceManagerPointer->GetLogicalDevice(), textureSampler, nullptr);
	vkDestroyImageView(deviceManagerPointer->GetLogicalDevice(), textureImageView, nullptr);

	vkDestroyImage(deviceManagerPointer->GetLogicalDevice(), textureImage, nullptr);
}

void TextureManager::createImageViews() {
	swapchainImageViews.resize(swapchainImages.size());

	for (uint32_t i = 0; i < swapchainImages.size(); i++) {
		swapchainImageViews[i] = createImageView(swapchainImages[i], swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

VkImageView TextureManager::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(deviceManagerPointer->GetLogicalDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

void TextureManager::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(deviceManagerPointer->GetLogicalDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(deviceManagerPointer->GetLogicalDevice(), image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = memoryManagerPointer->findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(deviceManagerPointer->GetLogicalDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(deviceManagerPointer->GetLogicalDevice(), image, imageMemory, 0);
}

void TextureManager::createTextureSampler() {
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(deviceManagerPointer->GetLogicalDevice(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

void TextureManager::createTextureImage(const std::string TEXTURE_PATH) {
	// The stbi_load function takes the file path and number of channels to load as arguments. 
	// The STBI_rgb_alpha value forces the image to be loaded with an alpha channel, even if it 
	// doesn't have one, which is nice for consistency with other textures in the future. The 
	// middle three parameters are outputs for the width, height and actual number of channels in the image. 
	// The pointer that is returned is the first element in an array of pixel values. The pixels are 
	// laid out row by row with 4 bytes per pixel in the case of STBI_rgb_alpha for a total of texWidth * texHeight * 4 values.
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = (uint64_t)texWidth * (uint64_t)texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	// create a buffer in host visible memory so that we can use vkMapMemory and copy the pixels to it
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	// The buffer should be in host visible memory so that we can map it and it should be usable as a transfer source so that we can copy it to an image later on
	bufferManagerPointer->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	// We can then directly copy the pixel values that we got from the image loading library to the buffer
	void* data;
	vkMapMemory(deviceManagerPointer->GetLogicalDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(deviceManagerPointer->GetLogicalDevice(), stagingBufferMemory);

	// dont forget to clean up the original pixel array now
	stbi_image_free(pixels);

	createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, memoryManagerPointer->GetTextureImageMemory());

	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// The image was created with the VK_IMAGE_LAYOUT_UNDEFINED layout, so that one should be specified as old layout when transitioning textureImage. Remember that we can do this because we don't care about its contents before performing the copy operation.
	bufferManagerPointer->copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

	// To be able to start sampling from the texture image in the shader, we need one last transition to prepare it for shader access
	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(deviceManagerPointer->GetLogicalDevice(), stagingBuffer, nullptr);
	vkFreeMemory(deviceManagerPointer->GetLogicalDevice(), stagingBufferMemory, nullptr);
}

void TextureManager::createTextureImageView() {
	textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void TextureManager::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkCommandBuffer commandBuffer = syncManagerPointer->beginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	syncManagerPointer->endSingleTimeCommands(commandBuffer);
}

VkFormat TextureManager::findDepthFormat() {
	return findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

VkFormat TextureManager::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(deviceManagerPointer->GetPhysicalDevice(), format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

void TextureManager::createDepthResources() {
	VkFormat depthFormat = findDepthFormat();

	createImage(swapchainManagerPointer->GetSwapchainExtent().width, swapchainManagerPointer->GetSwapchainExtent().height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, memoryManagerPointer->GetDepthImageMemory());
	depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}