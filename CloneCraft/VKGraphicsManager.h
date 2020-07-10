#pragma once
#include "GlobalHelpers.h"
#include "VKDeviceManager.h"
#include "Vertex.h"
#include <algorithm>
#include <array>
#include <unordered_map>
#include <chrono>

class VKGraphicsManager {
public:
	VKGraphicsManager(VKDeviceManager& deviceManager, VKSurfaceManager& surfaceManager, WindowManager& windowManager, const uint32_t _MAX_FRAMES_IN_FLIGHT);
	~VKGraphicsManager();
	void drawFrame();
	void CreateSwapChain();
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	void cleanupSwapChain();
	void createImageViews();
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void createRenderPass();
	VkFormat findDepthFormat();
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	void createGraphicsPipeline();
	VkShaderModule createShaderModule(const std::vector<char>& code);
	void createDepthResources();
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void createFrameBuffers();
	void createUniformBuffers();
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void createDescriptorPool();
	void createDescriptorSets();
	void createCommandBuffers();
	void LoadModel(const std::string MODEL_PATH);
	void createTextureSampler();
	void createTextureImage(const std::string TEXTURE_PATH);
	void createTextureImageView();
	void createVertexBuffer();
	void createIndexBuffer();
	void updateUniformBuffer(uint32_t currentImage);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void createCommandPool();
	void createSyncObjects();
	void createDescriptorSetLayout();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	VkCommandBuffer beginSingleTimeCommands();
	void recreateSwapChain();

	VkSwapchainKHR&				GetSwapChain()				{ return swapChain; }
	std::vector<VkImage>&		GetSwapChainImages()		{ return swapChainImages; }
	VkFormat&					GetSwapChainImageFormat()	{ return swapChainImageFormat; }
	VkExtent2D&					GetSwapChainExtent()		{ return swapChainExtent; }

private:
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	VkImage depthImage;
	VkImageView depthImageView;
	VkDeviceMemory depthImageMemory;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	VkImageView textureImageView;
	VkSampler textureSampler;
	std::vector<VkCommandBuffer> commandBuffers;
	VkCommandPool commandPool;
	VkBuffer vertexBuffer;
	VkBuffer indexBuffer;
	std::vector<uint32_t> indices;
	std::vector<Vertex> vertices;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	size_t currentFrame = 0; // keeps track of the current frame. no way! :D
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	VkDeviceMemory vertexBufferMemory;
	VkDeviceMemory indexBufferMemory;
	uint32_t MAX_FRAMES_IN_FLIGHT;

	VKDeviceManager* deviceManagerPointer;
	VKSurfaceManager* surfaceManagerPointer;
	WindowManager* windowManagerPointer;
};

