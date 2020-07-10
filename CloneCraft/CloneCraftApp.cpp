#include "CloneCraftApp.h"

CloneCraftApp::CloneCraftApp() :
	instanceManager(validationLayers),
	debugMessengerManager(instanceManager), 
	surfaceManager(instanceManager, windowManager), 
	deviceManager(instanceManager, surfaceManager, deviceExtensions, validationLayers),
	graphicsManager(deviceManager, surfaceManager, windowManager, MAX_FRAMES_IN_FLIGHT)
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
	graphicsManager.CreateSwapChain();
	graphicsManager.createImageViews();
	graphicsManager.createRenderPass();
	graphicsManager.createDescriptorSetLayout();
	graphicsManager.createGraphicsPipeline();
	graphicsManager.createDepthResources();
	graphicsManager.createFrameBuffers();
	graphicsManager.createCommandPool();
	graphicsManager.createTextureImage(TEXTURE_PATH);
	graphicsManager.createTextureImageView();
	graphicsManager.createTextureSampler();
	graphicsManager.LoadModel(MODEL_PATH);
	graphicsManager.createVertexBuffer();
	graphicsManager.createIndexBuffer();
	graphicsManager.createUniformBuffers();
	graphicsManager.createDescriptorPool();
	graphicsManager.createDescriptorSets();
	graphicsManager.createCommandBuffers();
	graphicsManager.createSyncObjects();
}

void CloneCraftApp::mainLoop() {
	while (!glfwWindowShouldClose(windowManager.GetWindow())) {
		glfwPollEvents();
		drawFrame();
	}

	vkDeviceWaitIdle(deviceManager.GetLogicalDevice()); // when exiting the loop, drawing and presentation operations may still be going on. Cleaning up resources while that is happening is a bad idea.
}

void CloneCraftApp::drawFrame() {
	graphicsManager.drawFrame();
}