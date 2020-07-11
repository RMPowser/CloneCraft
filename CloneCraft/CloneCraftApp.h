#pragma once
#include "WindowManager.h"
#include "InstanceManager.h"
#include "DebugMessengerManager.h"
#include "SurfaceManager.h"
#include "DeviceManager.h"
#include "SwapchainManager.h"
#include "RenderManager.h"
#include "PipelineManager.h"
#include "TextureManager.h"
#include "MemoryManager.h"
#include "BufferManager.h"
#include "SyncManager.h"
#include "DescriptorManager.h"
#include "ShaderManager.h"
#include "Camera.h"

class CloneCraftApp {
public:
	CloneCraftApp();
	~CloneCraftApp();
	void run();

private:
	const uint32_t WINDOW_WIDTH = 800;
	const uint32_t WINDOW_HEIGHT = 600;
	const uint32_t FOV = 75;
	const char* WINDOW_TITLE = "CloneCraft   :^)";
	const uint32_t MAX_FRAMES_IN_FLIGHT = 2; // defines how many frames should be processed concurrently. must be non-zero and positive
	const std::string MODEL_PATH = "models/viking_room.obj";
	const std::string TEXTURE_PATH = "textures/viking_room.png";
	const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };
	const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	WindowManager windowManager;
	InstanceManager instanceManager;
	DebugMessengerManager debugMessengerManager;
	SurfaceManager surfaceManager;
	DeviceManager deviceManager;
	SwapchainManager swapchainManager;
	RenderManager renderManager;
	PipelineManager pipelineManager;
	TextureManager textureManager;
	MemoryManager memoryManager;
	BufferManager bufferManager;
	SyncManager syncManager;
	DescriptorManager descriptorManager;
	ShaderManager shaderManager;
	const Camera camera;

	const Camera& getCamera();
	void initVulkan();
	void mainLoop();
	void drawFrame();
};