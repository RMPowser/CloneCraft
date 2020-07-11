#include "RenderManager.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>



////////////////////////////////// this section needs to be here so that the unordered map can copy construct a vertex ////////////////////////////
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


 
RenderManager::RenderManager(DeviceManager& deviceManager,
	SwapchainManager& swapchainManager,
	SyncManager& syncManager,
	BufferManager& bufferManager,
	WindowManager& windowManager,
	TextureManager& textureManager,
	SurfaceManager& surfaceManager) {
	deviceManagerPointer = &deviceManager;
	swapchainManagerPointer = &swapchainManager;
	syncManagerPointer = &syncManager;
	bufferManagerPointer = &bufferManager;
	windowManagerPointer = &windowManager;
	textureManagerPointer = &textureManager;
	surfaceManagerPointer = &surfaceManager;
}

RenderManager::~RenderManager() {
	vkDestroyCommandPool(deviceManagerPointer->GetLogicalDevice(), commandPool, nullptr);
}

void RenderManager::drawFrame() {
	vkWaitForFences(deviceManagerPointer->GetLogicalDevice(), 1, &((*syncManagerPointer).GetInFlightFences()[currentFrame]), VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(deviceManagerPointer->GetLogicalDevice(), swapchainManagerPointer->GetSwapChain(), UINT64_MAX, syncManagerPointer->GetImageAvailableSemaphores()[currentFrame], VK_NULL_HANDLE, &imageIndex);

	bufferManagerPointer->updateUniformBuffer(imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		swapchainManagerPointer->recreateSwapChain();
		return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swapchain image!");
	}

	// Check if a previous frame is using this image (i.e.there is its fence to wait on)
	if (syncManagerPointer->GetImagesInFlight()[imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(deviceManagerPointer->GetLogicalDevice(), 1, &(syncManagerPointer->GetImagesInFlight()[imageIndex]), VK_TRUE, UINT64_MAX);
	}

	// Mark the image as now being in use by this frame
	syncManagerPointer->GetImagesInFlight()[imageIndex] = syncManagerPointer->GetInFlightFences()[currentFrame];

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { syncManagerPointer->GetImageAvailableSemaphores()[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &(bufferManagerPointer->GetCommandBuffers()[imageIndex]);

	VkSemaphore signalSemaphores[] = { syncManagerPointer->GetRenderFinishedSemaphores()[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(deviceManagerPointer->GetLogicalDevice(), 1, &(syncManagerPointer->GetInFlightFences()[currentFrame]));

	if (vkQueueSubmit(deviceManagerPointer->GetGraphicsQueue(), 1, &submitInfo, syncManagerPointer->GetInFlightFences()[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapchainManagerPointer->GetSwapChain() };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional

	result = vkQueuePresentKHR(deviceManagerPointer->GetPresentQueue(), &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || windowManagerPointer->isFramebufferResized()) {
		windowManagerPointer->SetFramebufferResized(false);
		swapchainManagerPointer->recreateSwapChain();
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	// Increment the frame. By using the modulo(%) operator, we ensure that the frame index loops around after every MAX_FRAMES_IN_FLIGHT enqueued frames.
	currentFrame = (currentFrame + 1) % syncManagerPointer->GetMAX_FRAMES_IN_FLIGHT();
}





void RenderManager::createRenderPass() {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = textureManagerPointer->GetSwapchainImageFormat();
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = textureManagerPointer->findDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(deviceManagerPointer->GetLogicalDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}













void RenderManager::LoadModel(const std::string MODEL_PATH) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
		throw std::runtime_error(warn + err);
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}
}








void RenderManager::createCommandPool() {
	GlobalHelpers::QueueFamilyIndices queueFamilyIndices = GlobalHelpers::findQueueFamilies(deviceManagerPointer->GetPhysicalDevice(), surfaceManagerPointer->GetSurface());


	// Command buffers are executed by submitting them on one of the device queues, like the graphics and presentation queues we 
	// retrieved. Each command pool can only allocate command buffers that are submitted on a single type of queue. We're going 
	// to record commands for drawing, which is why we've chosen the graphics queue family.
	//
	// There are two possible flags for command pools :
	//
	//		VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands very often(may change memory allocation behavior)
	//		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT : Allow command buffers to be rerecorded individually, without this flag they all have to be reset together
	//
	// We will only record the command buffers at the beginning of the program and then execute them many times in the main loop, so we're not going to use either of these flags.
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	poolInfo.flags = 0; // Optional

	if (vkCreateCommandPool(deviceManagerPointer->GetLogicalDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}





