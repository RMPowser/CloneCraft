#include "SyncManager.h"
#include "TextureManager.h"

SyncManager::SyncManager(DeviceManager& deviceManager, TextureManager& textureManager, RenderManager& renderManager, uint32_t _MAX_FRAMES_IN_FLIGHT) {
	deviceManagerPointer = &deviceManager;
	textureManagerPointer = &textureManager;
	renderManagerPointer = &renderManager;
	MAX_FRAMES_IN_FLIGHT = _MAX_FRAMES_IN_FLIGHT;
}

SyncManager::~SyncManager() {
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(deviceManagerPointer->GetLogicalDevice(), renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(deviceManagerPointer->GetLogicalDevice(), imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(deviceManagerPointer->GetLogicalDevice(), inFlightFences[i], nullptr);
	}
}

void SyncManager::createSyncObjects() {
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	imagesInFlight.resize(textureManagerPointer->GetSwapchainImages().size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(deviceManagerPointer->GetLogicalDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(deviceManagerPointer->GetLogicalDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(deviceManagerPointer->GetLogicalDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}

VkCommandBuffer SyncManager::beginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = renderManagerPointer->GetCommandPool();
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(deviceManagerPointer->GetLogicalDevice(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void SyncManager::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(deviceManagerPointer->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(deviceManagerPointer->GetGraphicsQueue());

	vkFreeCommandBuffers(deviceManagerPointer->GetLogicalDevice(), renderManagerPointer->GetCommandPool(), 1, &commandBuffer);
}
