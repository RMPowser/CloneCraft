#pragma once
#include "GlobalHelpers.h"
#include "DeviceManager.h"
#include "SyncManager.h"
#include "WindowManager.h"
#include "TextureManager.h"
#include "SurfaceManager.h"
#include "Vertex.h"
#include <algorithm>
#include <array>
#include <unordered_map>
#include <chrono>

class SwapchainManager;
class BufferManager;

class RenderManager {
public:
	RenderManager(
		DeviceManager& deviceManager, 
		SwapchainManager& swapchainManager, 
		SyncManager& syncManager, 
		BufferManager& bufferManager, 
		WindowManager& windowManager, 
		TextureManager& textureManager,
		SurfaceManager& surfaceManager);
	~RenderManager();
	void drawFrame();
	void createRenderPass();
	void LoadModel(const std::string MODEL_PATH);
	void createCommandPool();

	VkRenderPass&			GetRenderPass()		{ return renderPass; }
	VkCommandPool&			GetCommandPool()	{ return commandPool; }
	std::vector<uint32_t>&	GetIndices()		{ return indices; }
	std::vector<Vertex>&	GetVertices()		{ return vertices; }

private:
	VkRenderPass renderPass;
	VkCommandPool commandPool;
	std::vector<uint32_t> indices;
	std::vector<Vertex> vertices;
	size_t currentFrame = 0; // keeps track of the current frame. no way! :D

	DeviceManager* deviceManagerPointer;
	SwapchainManager* swapchainManagerPointer;
	SyncManager* syncManagerPointer;
	BufferManager* bufferManagerPointer;
	WindowManager* windowManagerPointer;
	TextureManager* textureManagerPointer;
	SurfaceManager* surfaceManagerPointer;
};

