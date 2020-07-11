#include "BufferManager.h"

BufferManager::BufferManager(TextureManager& textureManager, SwapchainManager& swapchainManager, DeviceManager& deviceManager, MemoryManager& memoryManager, PipelineManager& pipelineManager, RenderManager& renderManager, DescriptorManager& descriptorManager, SyncManager& syncManager){
	textureManagerPointer = &textureManager;
	swapchainManagerPointer = &swapchainManager;
	deviceManagerPointer = &deviceManager;
	memoryManagerPointer = &memoryManager;
	pipelineManagerPointer = &pipelineManager;
	renderManagerPointer = &renderManager;
	descriptorManagerPointer = &descriptorManager;
	syncManagerPointer = &syncManager;
}

BufferManager::~BufferManager() {
	vkDestroyBuffer(deviceManagerPointer->GetLogicalDevice(), indexBuffer, nullptr);
	vkDestroyBuffer(deviceManagerPointer->GetLogicalDevice(), vertexBuffer, nullptr);
}

void BufferManager::createFrameBuffers() {
	swapchainFramebuffers.resize(textureManagerPointer->GetSwapchainImageViews().size()); // resize the container to hold all of the framebuffers

	for (size_t i = 0; i < textureManagerPointer->GetSwapchainImageViews().size(); i++) { // iterate through the image views and create framebuffers from them
		std::array<VkImageView, 2> attachments = { textureManagerPointer->GetSwapchainImageViews()[i], textureManagerPointer->GetDepthImageView() };

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderManagerPointer->GetRenderPass();
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapchainManagerPointer->GetSwapchainExtent().width;
		framebufferInfo.height = swapchainManagerPointer->GetSwapchainExtent().height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(deviceManagerPointer->GetLogicalDevice(), &framebufferInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void BufferManager::createUniformBuffers() {
	// We're going to write a separate function that updates the uniform buffer with a new transformation every frame, so there will be no vkMapMemory here.
	// The uniform data will be used for all draw calls, so the buffer containing it should only be destroyed when we stop rendering. 
	// Since it also depends on the number of swap chain images, which could change after a recreation, we'll clean it up in cleanupSwapChain
	VkDeviceSize bufferSize = sizeof(GlobalHelpers::UniformBufferObject);

	uniformBuffers.resize(textureManagerPointer->GetSwapchainImages().size());
	memoryManagerPointer->GetUniformBuffersMemory().resize(textureManagerPointer->GetSwapchainImages().size());

	for (size_t i = 0; i < textureManagerPointer->GetSwapchainImages().size(); i++) {
		createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], memoryManagerPointer->GetUniformBuffersMemory()[i]);
	}
}

void BufferManager::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(deviceManagerPointer->GetLogicalDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(deviceManagerPointer->GetLogicalDevice(), buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = memoryManagerPointer->findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(deviceManagerPointer->GetLogicalDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(deviceManagerPointer->GetLogicalDevice(), buffer, bufferMemory, 0);
}

void BufferManager::createCommandBuffers() {
	commandBuffers.resize(swapchainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = renderManagerPointer->GetCommandPool();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(deviceManagerPointer->GetLogicalDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	for (size_t i = 0; i < commandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderManagerPointer->GetRenderPass();
		renderPassInfo.framebuffer = swapchainFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapchainManagerPointer->GetSwapchainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManagerPointer->GetGraphicsPipeline());

		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManagerPointer->GetPipelineLayout(), 0, 1, &(descriptorManagerPointer->GetDescriptorSets()[i]), 0, nullptr);

		vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(renderManagerPointer->GetIndices().size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffers[i]);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}
}

void BufferManager::createVertexBuffer() {
	VkDeviceSize bufferSize = sizeof(renderManagerPointer->GetVertices()[0]) * renderManagerPointer->GetVertices().size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(deviceManagerPointer->GetLogicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, renderManagerPointer->GetVertices().data(), (size_t)bufferSize);
	vkUnmapMemory(deviceManagerPointer->GetLogicalDevice(), stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, memoryManagerPointer->GetVertexBufferMemory());

	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(deviceManagerPointer->GetLogicalDevice(), stagingBuffer, nullptr);
	vkFreeMemory(deviceManagerPointer->GetLogicalDevice(), stagingBufferMemory, nullptr);
}

void BufferManager::createIndexBuffer() {
	VkDeviceSize bufferSize = sizeof(renderManagerPointer->GetIndices()[0]) * renderManagerPointer->GetIndices().size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(deviceManagerPointer->GetLogicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, renderManagerPointer->GetIndices().data(), (size_t)bufferSize);
	vkUnmapMemory(deviceManagerPointer->GetLogicalDevice(), stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, memoryManagerPointer->GetIndexBufferMemory());

	copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(deviceManagerPointer->GetLogicalDevice(), stagingBuffer, nullptr);
	vkFreeMemory(deviceManagerPointer->GetLogicalDevice(), stagingBufferMemory, nullptr);
}

void BufferManager::updateUniformBuffer(uint32_t currentImage) {
	// start out with some logic to calculate the time in seconds since rendering has started with floating point accuracy.
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	// now define the model, view, and projection transformations in the uniform buffer object. The model rotation will be a 
	// simple rotation around the Z-axis using the time variable
	// The glm::rotate function takes an existing transformation, rotation angle and rotation axis as parameters. The 
	// glm::mat4(1.0f) constructor returns an identity matrix. Using a rotation angle of time * glm::radians(90.0f) accomplishes
	// the rotation of 90 degrees per second.
	GlobalHelpers::UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	// For the view transformation I've decided to look at the geometry from above at a 45 degree angle. The glm::lookAt function
	// takes the eye position, center position and up axis as parameters.
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	// I've chosen to use a perspective projection with a 45 degree vertical field-of-view. The other parameters are the aspect 
	// ratio, near and far view planes. It is important to use the current swap chain extent to calculate the aspect ratio to 
	// take into account the new width and height of the window after a resize.
	ubo.proj = glm::perspective(glm::radians(45.0f), swapchainManagerPointer->GetSwapchainExtent().width / (float)swapchainManagerPointer->GetSwapchainExtent().height, 0.1f, 10.0f);

	// GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted. The easiest way to 
	// compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix. If you don't do 
	// this, then the image will be rendered upside down.
	ubo.proj[1][1] *= -1;

	// All of the transformations are defined now, so we can copy the data in the uniform buffer object to the current uniform
	// buffer. This happens in exactly the same way as we did for vertex buffers, except without a staging buffer
	void* data;
	vkMapMemory(deviceManagerPointer->GetLogicalDevice(), memoryManagerPointer->GetUniformBuffersMemory()[currentImage], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(deviceManagerPointer->GetLogicalDevice(), memoryManagerPointer->GetUniformBuffersMemory()[currentImage]);
}

void BufferManager::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer commandBuffer = syncManagerPointer->beginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	syncManagerPointer->endSingleTimeCommands(commandBuffer);
}


void BufferManager::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
	VkCommandBuffer commandBuffer = syncManagerPointer->beginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	syncManagerPointer->endSingleTimeCommands(commandBuffer);
}