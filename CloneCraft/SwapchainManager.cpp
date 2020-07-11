#include "SwapchainManager.h"

SwapchainManager::SwapchainManager(
	DeviceManager& deviceManager,
	SurfaceManager& surfaceManager,
	WindowManager& windowManager,
	TextureManager& textureManager,
	MemoryManager& memoryManager,
	PipelineManager& pipelineManager,
	BufferManager& bufferManager,
	RenderManager& renderManager,
	DescriptorManager& descriptorManager) 
	{
	deviceManagerPointer = &deviceManager;
	surfaceManagerPointer = &surfaceManager;
	windowManagerPointer = &windowManager;
	textureManagerPointer = &textureManager;
	memoryManagerPointer = &memoryManager;
	pipelineManagerPointer = &pipelineManager;
	bufferManagerPointer = &bufferManager;
	renderManagerPointer = &renderManager;
	descriptorManagerPointer = &descriptorManager;
}

SwapchainManager::~SwapchainManager() {
	cleanupSwapChain();
}

void SwapchainManager::createSwapChain() {
	GlobalHelpers::SwapChainSupportDetails swapChainSupport = GlobalHelpers::querySwapChainSupport(deviceManagerPointer->GetPhysicalDevice(), surfaceManagerPointer->GetSurface());

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = (*surfaceManagerPointer).GetSurface();

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1; // specifies the amount of layers each image consists of. always 1 unless developing a stereoscopic 3D application.
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // specifies what kind of operations we'll use the images in the swap chain for. render directly to them for now, which means that they're used as color attachment. also possible to render images to a separate image first to perform operations like post-processing. In that case you may use a value like VK_IMAGE_USAGE_TRANSFER_DST_BIT instead and use a memory operation to transfer the rendered image to a swap chain image.

	GlobalHelpers::QueueFamilyIndices indices = GlobalHelpers::findQueueFamilies(deviceManagerPointer->GetPhysicalDevice(), surfaceManagerPointer->GetSurface());
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //The compositeAlpha field specifies if the alpha channel should be used for blending with other windows in the window system. You'll almost always want to simply ignore the alpha channel, hence VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR.
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(deviceManagerPointer->GetLogicalDevice(), &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(deviceManagerPointer->GetLogicalDevice(), swapchain, &imageCount, nullptr);
	textureManagerPointer->GetSwapchainImages().resize(imageCount);
	vkGetSwapchainImagesKHR(deviceManagerPointer->GetLogicalDevice(), swapchain, &imageCount, textureManagerPointer->GetSwapchainImages().data());

	textureManagerPointer->GetSwapchainImageFormat() = surfaceFormat.format;
	swapchainExtent = extent;
}

void SwapchainManager::cleanupSwapChain() {
	vkDestroyImageView(deviceManagerPointer->GetLogicalDevice(), textureManagerPointer->GetDepthImageView(), nullptr);
	vkDestroyImage(deviceManagerPointer->GetLogicalDevice(), textureManagerPointer->GetDepthImage(), nullptr);
	vkFreeMemory(deviceManagerPointer->GetLogicalDevice(), memoryManagerPointer->GetDepthImageMemory(), nullptr);

	for (size_t i = 0; i < bufferManagerPointer->GetSwapchainFramebuffers().size(); i++) {
		vkDestroyFramebuffer(deviceManagerPointer->GetLogicalDevice(), bufferManagerPointer->GetSwapchainFramebuffers()[i], nullptr);
	}

	vkFreeCommandBuffers(deviceManagerPointer->GetLogicalDevice(), renderManagerPointer->GetCommandPool(), static_cast<uint32_t>(bufferManagerPointer->GetCommandBuffers().size()), bufferManagerPointer->GetCommandBuffers().data());

	vkDestroyPipeline(deviceManagerPointer->GetLogicalDevice(), pipelineManagerPointer->GetGraphicsPipeline(), nullptr);
	vkDestroyPipelineLayout(deviceManagerPointer->GetLogicalDevice(), pipelineManagerPointer->GetPipelineLayout(), nullptr);
	vkDestroyRenderPass(deviceManagerPointer->GetLogicalDevice(), renderManagerPointer->GetRenderPass(), nullptr);

	for (size_t i = 0; i < textureManagerPointer->GetSwapchainImageViews().size(); i++) {
		vkDestroyImageView(deviceManagerPointer->GetLogicalDevice(), textureManagerPointer->GetSwapchainImageViews()[i], nullptr);
	}

	vkDestroySwapchainKHR(deviceManagerPointer->GetLogicalDevice(), swapchain, nullptr);

	// uniform buffers depends on the number of images in the swapchain so it is cleaned up here. dont forget to recreate it in recreateSwapChain()
	for (size_t i = 0; i < textureManagerPointer->GetSwapchainImages().size(); i++) {
		vkDestroyBuffer(deviceManagerPointer->GetLogicalDevice(), bufferManagerPointer->GetUniformBuffers()[i], nullptr);
		vkFreeMemory(deviceManagerPointer->GetLogicalDevice(), memoryManagerPointer->GetUniformBuffersMemory()[i], nullptr);
	}

	// The descriptor pool should be destroyed when the swap chain is recreated because it depends on the number of images. recreate it in recreateSwapChain().
	vkDestroyDescriptorPool(deviceManagerPointer->GetLogicalDevice(), descriptorManagerPointer->GetDescriptorPool(), nullptr);

}

void SwapchainManager::recreateSwapChain() {
	int width = 0, height = 0;
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(windowManagerPointer->GetWindow(), &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(deviceManagerPointer->GetLogicalDevice());

	cleanupSwapChain();

	createSwapChain();
	textureManagerPointer->createImageViews();
	renderManagerPointer->createRenderPass();
	pipelineManagerPointer->createGraphicsPipeline();
	textureManagerPointer->createDepthResources();
	bufferManagerPointer->createFrameBuffers();
	bufferManagerPointer->createUniformBuffers();
	descriptorManagerPointer->createDescriptorPool();
	descriptorManagerPointer->createDescriptorSets();
	bufferManagerPointer->createCommandBuffers();
}

VkSurfaceFormatKHR SwapchainManager::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	// check available color spaces and prefer sRGB
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR SwapchainManager::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapchainManager::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	} else {
		int width, height;
		glfwGetFramebufferSize(windowManagerPointer->GetWindow(), &width, &height);

		VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}
