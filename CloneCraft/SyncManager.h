#pragma once
#include "vulkan/vulkan.h"
#include "DeviceManager.h"
#include <vector>

class TextureManager;
class RenderManager;

class SyncManager {
public:
	SyncManager(DeviceManager& deviceManager, TextureManager& textureManager, RenderManager& renderManager, uint32_t _MAX_FRAMES_IN_FLIGHT);
	~SyncManager();
	void createSyncObjects();
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	std::vector<VkSemaphore>& GetImageAvailableSemaphores() { return imageAvailableSemaphores; }
	std::vector<VkSemaphore>& GetRenderFinishedSemaphores() { return renderFinishedSemaphores; }
	std::vector<VkFence>& GetInFlightFences() { return inFlightFences; }
	std::vector<VkFence>& GetImagesInFlight() { return imagesInFlight; }
	uint32_t& GetMAX_FRAMES_IN_FLIGHT() { return MAX_FRAMES_IN_FLIGHT; }

private:
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	uint32_t MAX_FRAMES_IN_FLIGHT;

	DeviceManager* deviceManagerPointer;
	TextureManager* textureManagerPointer;
	RenderManager* renderManagerPointer;
};