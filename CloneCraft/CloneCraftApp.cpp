#include "CloneCraftApp.h"

CloneCraftApp::CloneCraftApp() :
	instanceManager(validationLayers),
	debugMessengerManager(instanceManager), 
	surfaceManager(instanceManager, windowManager), 
	deviceManager(instanceManager, surfaceManager, deviceExtensions, validationLayers),
	renderManager(deviceManager, swapchainManager, syncManager, bufferManager, windowManager, textureManager, surfaceManager),
	pipelineManager(deviceManager, swapchainManager, shaderManager, descriptorManager, renderManager),
	textureManager(deviceManager, swapchainManager, memoryManager, bufferManager, syncManager),
	memoryManager(deviceManager),
	swapchainManager(deviceManager, surfaceManager, windowManager, textureManager, memoryManager, pipelineManager, bufferManager, renderManager, descriptorManager),
	bufferManager(textureManager, swapchainManager, deviceManager, memoryManager, pipelineManager, renderManager, descriptorManager, syncManager),
	syncManager(deviceManager, textureManager, renderManager, MAX_FRAMES_IN_FLIGHT),
	descriptorManager(deviceManager, textureManager, bufferManager),
	shaderManager(deviceManager)
{}

CloneCraftApp::~CloneCraftApp() {
}

void CloneCraftApp::run() {
	windowManager.CreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
	initVulkan();
	mainLoop();
}

const Camera& CloneCraftApp::getCamera() {
	return camera;
}

void CloneCraftApp::initVulkan() {
	instanceManager.CreateVKInstance();
	debugMessengerManager.SetupDebugMessenger();
	surfaceManager.CreateSurface();
	deviceManager.PickPhysicalDevice();
	deviceManager.CreateLogicalDevice();
	swapchainManager.createSwapChain();
	textureManager.createImageViews();
	renderManager.createRenderPass();
	descriptorManager.createDescriptorSetLayout();
	pipelineManager.createGraphicsPipeline();
	textureManager.createDepthResources();
	bufferManager.createFrameBuffers();
	renderManager.createCommandPool();
	textureManager.createTextureImage(TEXTURE_PATH);
	textureManager.createTextureImageView();
	textureManager.createTextureSampler();
	renderManager.LoadModel(MODEL_PATH);
	bufferManager.createVertexBuffer();
	bufferManager.createIndexBuffer();
	bufferManager.createUniformBuffers();
	descriptorManager.createDescriptorPool();
	descriptorManager.createDescriptorSets();
	bufferManager.createCommandBuffers();
	syncManager.createSyncObjects();
}

void CloneCraftApp::mainLoop() {
	while (!glfwWindowShouldClose(windowManager.GetWindow())) {
		glfwPollEvents();
		drawFrame();
	}

	vkDeviceWaitIdle(deviceManager.GetLogicalDevice()); // when exiting the loop, drawing and presentation operations may still be going on. Cleaning up resources while that is happening is a bad idea.
}

void CloneCraftApp::drawFrame() {
	renderManager.drawFrame();
}