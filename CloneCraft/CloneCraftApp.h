#pragma once
#include <Windows.h>
#include "vulkan/vulkan.h"
#include "MatrixFunctions.h"
#include "AppConfig.h"
#include "Camera.h"
#include "Player.h"
#include "World.h"
#include "Chunk.h"
#include "Block.h"
#include "UBO.h"
#include "glm.h"
#include "Ray.h"
#include "Controller.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <optional>
#include <set>
#include <cstdint> // Necessary for UINT32_MAX
#include <algorithm>
#include <glm/glm.hpp>
#include <array>
#include <cmath>






/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// define globals
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Player player;
Controller controller;
bool cameraMat4Updated = false;
bool playerMat4Updated = false;
bool firstMouse = true;
float sensitivity = 0.1f;
uint8_t renderDistance = 2; // render distance in chunks
uint8_t ASYNC_NUM_CHUNKS_PER_FRAME = 2; // max number of chunks to load per frame


float moveAcceleration; // set this in the update function
float buildRange = 5;
float jumpHeight = 12; // this just happens to feel like a good number at 60fps

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// creates the dubug messenger to be used by the validation layers by getting a function pointer
// 
// CreateDebugUtilsMessengerEXT parameters:
//      instance: the instance the messenger will be used with
//      pCreateInfo: pointer to a VkDebugUtilsMessengerCreateInfoEXT structure containing the callback pointer, as well as defining conditions under which this messenger will trigger the callback
//      pAllocator: (always NULL in this program) controls host memory allocation as described in the Memory Allocation chapter. https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#memory-allocation
//      pDebugMessenger: pointer to a VkDebugUtilsMessengerEXT handle in which the created object is returned.
//
// vkGetInstanceProcAddr parameters: 
//      instance: instance that the function pointer will be compatible with, or NULL for commands not dependent on any instance
//      pName: name of the command to obtain
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VkResult CreateDebugUtilsMessengerEXT( VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// class containing the application.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CloneCraftApplication {
public:
	CloneCraftApplication(GLFWwindow* _window, AppConfig* _config) :
	camera(*_config),
	world(&blockdb, renderDistance, ASYNC_NUM_CHUNKS_PER_FRAME, _config->seed) {
		config = _config;
		#ifdef NDEBUG
		config->validationLayers = {};
		#else
		config->validationLayers = { "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };
		#endif
		
		camera.hookEntity(player);
		window = _window;
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}

	void run() {
		player.world = &world;
		initVulkan();

#ifndef NDEBUG
		//system("CLS");
#endif // !NDEBUG
		
		mainLoop();
		cleanup();
	}

private:
	AppConfig*						config;
	Camera							camera;
	BlockDatabase					blockdb;
	World							world;

	GLFWwindow*						window;
	VkInstance						instance;
	VkDebugUtilsMessengerEXT		debugMessenger;
	VkSurfaceKHR					surface;
	VkPhysicalDevice				physicalDevice = VK_NULL_HANDLE;
	VkDevice						device;
	VkQueue							graphicsQueue;
	VkQueue							presentQueue;
	VkSwapchainKHR					swapChain;
	std::vector<VkImage>			swapChainImages;
	VkFormat						swapChainImageFormat;
	VkExtent2D						swapChainExtent;
	std::vector<VkImageView>		swapChainImageViews;
	VkRenderPass					renderPass;
	VkDescriptorSetLayout			descriptorSetLayout;
	VkPipelineLayout				pipelineLayout;
	VkPipeline						graphicsPipeline;
	std::vector<VkFramebuffer>		swapChainFramebuffers;
	VkCommandPool					commandPool;
	std::vector<VkCommandBuffer>	commandBuffers; // Command buffers will be automatically freed when their command pool is destroyed, so we don't need an explicit cleanup.
	std::vector<VkSemaphore>		imageAvailableSemaphores;
	std::vector<VkSemaphore>		renderFinishedSemaphores;
	std::vector<VkFence>			inFlightFences;
	std::vector<VkFence>			imagesInFlight;
	size_t							currentFrame = 0; // keeps track of the current frame. no way! :D
	bool							framebufferResized = false;
	std::vector<Vertex>				vertices;
	std::vector<uint32_t>			indices;
	VkBuffer						vertexBuffer;
	VkDeviceMemory					vertexBufferMemory;
	VkBuffer						indexBuffer;
	VkDeviceMemory					indexBufferMemory;
	std::vector<VkBuffer>			uniformBuffers;
	std::vector<VkDeviceMemory>		uniformBuffersMemory;
	VkDescriptorPool				descriptorPool;
	std::vector<VkDescriptorSet>	descriptorSets;
	std::vector<VkImage>			textureImages;
	std::vector<VkDeviceMemory>		textureImagesMemory;
	std::vector<VkImageView>		textureImageViews;
	VkSampler						textureSampler;
	VkImage							depthImage;
	VkDeviceMemory					depthImageMemory;
	VkImageView						depthImageView;

	// The reason that we're creating a static function as a callback is because GLFW does not know how to properly call a member 
	// function with the right 'this' pointer to our HelloTriangleApplication instance. However, we do get a reference to the 
	// GLFWwindow in the callback and there is another GLFW function that allows you to store an arbitrary pointer inside of it called
	// glfwSetWindowUserPointer().
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<CloneCraftApplication*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// initialize vulkan using an order of functions
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void initVulkan() {
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createDepthResources();
		createFrameBuffers();
		createCommandPool();
		createTextureImages();
		createTextureSampler();
		//updateVertexAndIndexBuffer(); // this happens inside the world object now
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createCommandBuffers();
		createSyncObjects();
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// main loop
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void mainLoop() {
		static auto startTime = std::chrono::high_resolution_clock::now();

		while (!glfwWindowShouldClose(window)) {
			auto currentTime = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
			startTime = std::chrono::high_resolution_clock::now();

			update(time);
			glfwPollEvents();
			drawFrame(time);
		}

		vkDeviceWaitIdle(device); // when exiting the loop, drawing and presentation operations may still be going on. Cleaning up resources while that is happening is a bad idea.
	}

	void update(float dt) {
		auto acceleration = &(player.acceleration);
		auto rotation = &(player.rotation);

		if (controller.forwardPressed) {
			acceleration->x += -cos(RADIAN * (rotation->y + 90.f)) * moveAcceleration;
			acceleration->z += -sin(RADIAN * (rotation->y + 90.f)) * moveAcceleration;
		}
		if (controller.backPressed) {
			acceleration->x += cos(RADIAN * (rotation->y + 90.f)) * moveAcceleration;
			acceleration->z += sin(RADIAN * (rotation->y + 90.f)) * moveAcceleration;
		}
		if (controller.leftPressed) {
			acceleration->x += -cos(RADIAN * (rotation->y)) * moveAcceleration;
			acceleration->z += -sin(RADIAN * (rotation->y)) * moveAcceleration;
		}
		if (controller.rightPressed) {
			acceleration->x += cos(RADIAN * (rotation->y)) * moveAcceleration;
			acceleration->z += sin(RADIAN * (rotation->y)) * moveAcceleration;
		}
		if (controller.upPressed) {
			if (player.isOnGround && !player.isFlying) {
				acceleration->y += jumpHeight;
			}
			else if (player.isFlying) {
				acceleration->y += moveAcceleration;
			}
		}
		if (controller.downPressed) {
			acceleration->y += -moveAcceleration;
		}
		if (controller.flyToggleNew != controller.flyToggleOld) {
			player.isFlying = controller.flyToggleNew;
			controller.flyToggleOld = controller.flyToggleNew;
		}
		if (controller.speedModifierPressed) {
			moveAcceleration = 1.5;
		} else {
			moveAcceleration = 1;
		}
		if (controller.leftClicked) {
			Vec3 camPosition{};
			camPosition.x = camera.position.x;
			camPosition.y = camera.position.y;
			camPosition.z = camera.position.z;

			for (Ray ray(camPosition, player.rotation); ray.getLength() <= buildRange; ray.step(0.05f)) {
				int x = static_cast<int>(ray.getEnd().x);
				int y = static_cast<int>(ray.getEnd().y);
				int z = static_cast<int>(ray.getEnd().z);

				auto block = world.getBlock(x, y, z);

				if (block != BlockId::Air) {
					if (world.setBlock(BlockId::Air, x, y, z)) {
						world.updateChunk(World::getChunkXZ(x, z));
						break;
					} else {
						__debugbreak();
						throw new std::runtime_error("unable to destroy block!");
					}
				}
			}

			controller.leftClicked = false;
		}
		if (controller.rightClicked) {
			Vec3 camPosition{};
			camPosition.x = camera.position.x;
			camPosition.y = camera.position.y;
			camPosition.z = camera.position.z;

			Vec3 lastRayPosition;

			for (Ray ray(camPosition, player.rotation); ray.getLength() <= buildRange; ray.step(0.05f)) {
				int x = static_cast<int>(ray.getEnd().x);
				int y = static_cast<int>(ray.getEnd().y);
				int z = static_cast<int>(ray.getEnd().z);

				auto block = world.getBlock(x, y, z);

				if (block != BlockId::Air) {
					if (world.setBlock(BlockId::Grass, lastRayPosition.x, lastRayPosition.y, lastRayPosition.z)) {
						world.updateChunk(World::getChunkXZ(lastRayPosition.x, lastRayPosition.z));
						break;
					} else {
						__debugbreak();
						throw new std::runtime_error("unable to destroy block!");
					}
				}
				lastRayPosition = ray.getEnd();
			}
			controller.rightClicked = false;
		}



		player.update(dt, controller);
		camera.update();


#ifndef NDEBUG
		SetStdOutCursorPosition(0, 0);
		std::cout << "                                                               ";
		std::cout << "\n                                                               ";
		std::cout << "\n                                                               ";
		std::cout << "\n                                                               ";
		std::cout << "\n                                                               ";
		std::cout << "\n                                                               ";
		std::cout << "\n                                                               ";
		SetStdOutCursorPosition(0, 0);
		std::cout << "Player";
		std::cout << "\nx: " << player.bbox.position.x << "\ty: " << player.bbox.position.y << "\tz: " << player.bbox.position.z;
		std::cout << "\nspeed: " << moveAcceleration;
		std::cout << "\nacceleration x: " << acceleration->x << " y: " << acceleration->y << " z: " << acceleration->z;
		std::cout << "\nvelocity x: " << player.velocity.x << " y: " << player.velocity.y << " z: " << player.velocity.z;
		std::cout << "\ndt: " << dt << "\n";
#endif // DEBUG
		

		

		world.update(camera, physicalDevice, device, commandPool, graphicsQueue, vertices, vertexBuffer, vertexBufferMemory, indices, indexBuffer, indexBufferMemory);
	}

	void SetStdOutCursorPosition(short CoordX, short CoordY)
		//our function to set the cursor position.
	{
		HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		COORD position = { CoordX,CoordY };

		SetConsoleCursorPosition(hStdout, position);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Function will perform the following operations:
	//
	//		Acquire an image from the swap chain
	//		Execute the command buffer with that image as attachment in the framebuffer
	//		Return the image to the swap chain for presentation
	//
	// Each of these events is set in motion using a single function call, but they are executed asynchronously. The function calls 
	// will return before the operations are actually finished and the order of execution is also undefined. Unfortunate, because 
	// each of the operations depends on the previous one finishing. There are two ways of synchronizing swap chain events: fences 
	// and semaphores. They're both objects that can be used for coordinating operations by having one operation signal and another 
	// operation wait for a fence or semaphore to go from the unsignaled to signaled state. The difference is that the state of 
	// fences can be accessed from your program using calls like vkWaitForFences and semaphores cannot be. Fences are mainly designed
	// to synchronize your application itself with rendering operation, whereas semaphores are used to synchronize operations within
	// or across command queues. We want to synchronize the queue operations of draw commands and presentation, which makes 
	// semaphores the best fit.
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void drawFrame(float dt) {
		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		updateUniformBuffer(imageIndex, dt);
		
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		// Check if a previous frame is using this image (i.e.there is its fence to wait on)
		if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
			vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
		}
		// Mark the image as now being in use by this frame
		imagesInFlight[imageIndex] = inFlightFences[currentFrame];



		

		// Create the Command Buffer's Begin Info
		VkCommandBufferBeginInfo cmdBeginInfo = {};
		cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		cmdBeginInfo.pInheritanceInfo = nullptr;
		if (vkBeginCommandBuffer(commandBuffers[imageIndex], &cmdBeginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		// Setup the Render Pass
		VkRenderPassBeginInfo rndBeginInfo = {};

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { (70.0f / 255), (160.0f / 255), (255.0f / 255), (255.0f / 255) };
		clearValues[1].depthStencil = { 1.0f, 0 };

		rndBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rndBeginInfo.renderPass = renderPass;
		rndBeginInfo.framebuffer = swapChainFramebuffers[imageIndex];
		rndBeginInfo.renderArea.extent = swapChainExtent;
		rndBeginInfo.clearValueCount = clearValues.size();
		rndBeginInfo.pClearValues = clearValues.data();

		// Begin the Render Pass
		vkCmdBeginRenderPass(commandBuffers[imageIndex], &rndBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		

			vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			VkBuffer vertexBuffers[] = { vertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffers[imageIndex], 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(commandBuffers[imageIndex], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[imageIndex], 0, nullptr);

			vkCmdDrawIndexed(commandBuffers[imageIndex], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);


		// stop the render pass
		vkCmdEndRenderPass(commandBuffers[imageIndex]);



		// stop the command buffer
		if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}

		// setup the queue submit info
		VkSubmitInfo submitInfo = {};
		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		// reset the fence
		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT32_MAX);
		vkResetFences(device, 1, &inFlightFences[currentFrame]);

		// submit queue
		result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		//Setup the Present Info
		VkPresentInfoKHR present_info = {};
		VkSwapchainKHR swapChains[] = { swapChain };
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = signalSemaphores;
		present_info.swapchainCount = 1;
		present_info.pSwapchains = swapChains;
		present_info.pImageIndices = &imageIndex;
		present_info.pResults = nullptr; // optional

		//Present onto the surface
		result = vkQueuePresentKHR(presentQueue, &present_info);

		// Error Check for Swapchain and VSync Changes
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
			framebufferResized = false;
			recreateSwapChain();
		} else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
			__debugbreak();
		}

		// Increment the frame. By using the modulo(%) operator, we ensure that the frame index loops around after every MAX_FRAMES_IN_FLIGHT enqueued frames.
		currentFrame = (currentFrame + 1) % config->maxFramesInFlight;


	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// clean up any memory in reverse order of declaration
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void cleanup() {
		cleanupSwapChain();

		vkDestroySampler(device, textureSampler, nullptr);
		
		for (size_t i = 0; i < textureImages.size(); i++) {
			vkDestroyImageView(device, textureImageViews[i], nullptr);
			vkDestroyImage(device, textureImages[i], nullptr);
			vkFreeMemory(device, textureImagesMemory[i], nullptr);
		}
		
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vkDestroyBuffer(device, indexBuffer, nullptr);
		vkFreeMemory(device, indexBufferMemory, nullptr);

		vkDestroyBuffer(device, vertexBuffer, nullptr);
		vkFreeMemory(device, vertexBufferMemory, nullptr);

		for (size_t i = 0; i < config->maxFramesInFlight; i++) {
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}

		vkDestroyCommandPool(device, commandPool, nullptr);

		vkDestroyDevice(device, nullptr);

		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);

		glfwTerminate();
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// create an instance of vulkan and assign it to "instance" data member
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void createInstance() {
		try {
			if (!checkValidationLayerSupport()) {
				throw std::runtime_error("validation layers requested, but not available!\n");
			}
		} catch (std::runtime_error& e) {
			std::cerr << e.what();
			std::terminate();
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "CloneCraft";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "CloneCraft Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		auto extensions = getRequiredExtensions();

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		createInfo.enabledLayerCount = static_cast<uint32_t>(config->validationLayers.size());
		createInfo.ppEnabledLayerNames = config->validationLayers.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;


		try {
			if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
				throw std::runtime_error("failed to create vulkan instance\n");
			}
		} catch (std::runtime_error& e) {
			std::cerr << e.what();
			std::terminate();
		}
	}
		
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// populate the struct to use for creating the debug messenger 
	// parameters: 
	//      createInfo: struct to fill
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// create the debug messenger to use with the instance of vulkan if in debug mode using struct filled from populateDebugMessengerCreateInfo()
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void setupDebugMessenger() {
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// get and store a list of required extensions for glfw to function
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	std::vector<const char*> getRequiredExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		std::cout << "available extensions:\n";
		for (auto extension : extensions) {
			std::cout << "\t" << extension << std::endl;
		}
		return extensions;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// check if we have the proper validation layers and print them to the console
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool checkValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : config->validationLayers) {
			std::cout << "searching for: " << layerName << "\n";
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				std::cout << "\t" << layerProperties.layerName << "\n";
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					std::cout << "\t" << "layer found :D" << "\n";
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// pick a suitable device with vulkan support
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void pickPhysicalDevice() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPU with Vulkan support");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		for (auto& device : devices) {
			if (isDeviceSuitable(device)) {
				physicalDevice = device;
				VkPhysicalDeviceProperties prop;
				vkGetPhysicalDeviceProperties(physicalDevice, &prop);
				std::cout << "physical device found: " << prop.deviceName << "\n";
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find suitable GPU");
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// create a logical device to interface with the physical device
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void createLogicalDevice() {
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(config->deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = config->deviceExtensions.data();

		createInfo.enabledLayerCount = static_cast<uint32_t>(config->validationLayers.size());
		createInfo.ppEnabledLayerNames = config->validationLayers.data();

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}

		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// make sure the queuefamilies actually exist, the required extensions are supported, and the swap chain has at least one
	// supported image format and one supported presentation mode given the window surface we have
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool isDeviceSuitable(VkPhysicalDevice device) {
		QueueFamilyIndices indices = findQueueFamilies(device);

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

		return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// enumerate extensions and check if all required extentions are among them
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(config->deviceExtensions.begin(), config->deviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// populates the SwapChainSupportDetails struct with all relevant information
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}


		return details;
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// gets the queuefamilies from the passed in device and returns a struct containing them
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}

			i++;
		}

		return indices;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// create a surface for the vulkan instance to render to
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void createSurface() {
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// create the swap chain using helper functions 
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void createSwapChain() {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1; // specifies the amount of layers each image consists of. always 1 unless developing a stereoscopic 3D application.
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // specifies what kind of operations we'll use the images in the swap chain for. render directly to them for now, which means that they're used as color attachment. also possible to render images to a separate image first to perform operations like post-processing. In that case you may use a value like VK_IMAGE_USAGE_TRANSFER_DST_BIT instead and use a memory operation to transfer the rendered image to a swap chain image.

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
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

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// define the surface format to be used by the swapchain. sRGB preferred, otherwise go with the first one specified
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {

		// check available color spaces and prefer sRGB
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR) { // VK_PRESENT_MODE_FIFO_KHR is for vsync
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != UINT32_MAX) {
			return capabilities.currentExtent;
		} else {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// creates a basic image view for every image in the swap chain to use as color targets
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void createImageViews() {
		swapChainImageViews.resize(swapChainImages.size());

		for (uint32_t i = 0; i < swapChainImages.size(); i++) {
			swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		}
	}

	void createTextureSampler() {
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_NEAREST;
		samplerInfo.minFilter = VK_FILTER_NEAREST;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 16.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// We need to tell Vulkan about the framebuffer attachments that will be used while rendering. We need to specify how many color 
	// and depth buffers there will be, how many samples to use for each of them and how their contents should be handled throughout
	// the rendering operations. All of this information is wrapped in a render pass object.
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void createRenderPass() {
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = findDepthFormat();
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

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}

	void createTextureImages() {
		for (int i = 1; i < (int)BlockId::NUM_TYPES; i++) {
			int index = i - 1; // this is the index for the textureImages[] array, not the BlockId's
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;

			auto imageSize = blockdb.blockDataFor((BlockId)i).getTexture().size;
			auto pixels = blockdb.blockDataFor((BlockId)i).getTexture().image;
			auto texWidth = blockdb.blockDataFor((BlockId)i).getTexture().width;
			auto texHeight = blockdb.blockDataFor((BlockId)i).getTexture().height;

			// The buffer should be in host visible memory so that we can map it and it should be usable as a transfer source so that we can copy it to an image later on
			createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

			// We can then directly copy the pixel values that we got from the image loading library to the buffer
			void* data;
			vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
			memcpy(data, pixels, static_cast<size_t>(imageSize));
			vkUnmapMemory(device, stagingBufferMemory);

			VkImage image;
			VkDeviceMemory imageMemory;
			createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);

			textureImages.push_back(image);
			textureImagesMemory.push_back(imageMemory);

			transitionImageLayout(textureImages[index], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			// The image was created with the VK_IMAGE_LAYOUT_UNDEFINED layout, so that one should be specified as old layout when transitioning textureImage. Remember that we can do this because we don't care about its contents before performing the copy operation.
			copyBufferToImage(stagingBuffer, textureImages[index], static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

			// To be able to start sampling from the texture image in the shader, we need one last transition to prepare it for shader access
			transitionImageLayout(textureImages[index], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			vkDestroyBuffer(device, stagingBuffer, nullptr);
			vkFreeMemory(device, stagingBufferMemory, nullptr);

			VkImageView imageView = createImageView(textureImages[index], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

			textureImageViews.push_back(imageView);
		}
	}

	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}

		return imageView;
	}

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate image memory!");
		}

		vkBindImageMemory(device, image, imageMemory, 0);
	}

	VkCommandBuffer beginSingleTimeCommands() {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	void createDescriptorSetLayout() {
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.pImmutableSamplers = nullptr;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	void createDepthResources() {
		VkFormat depthFormat = findDepthFormat();

		createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
		depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	}

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}

		throw std::runtime_error("failed to find supported format!");
	}

	bool hasStencilComponent(VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	VkFormat findDepthFormat() {
		return findSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// create the graphics pipeline. this is the big one where everything comes together
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void createGraphicsPipeline() {
		auto vertShaderCode = readFile("shaders/vert.spv");
		auto fragShaderCode = readFile("shaders/frag.spv");

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_MAX;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = VK_NULL_HANDLE;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);

		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
			__debugbreak();
		}

		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertShaderModule, nullptr);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// create the framebuffers corresponding to each retrieved image view from the swap chain
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void createFrameBuffers() {
		swapChainFramebuffers.resize(swapChainImageViews.size()); // resize the container to hold all of the framebuffers

		for (size_t i = 0; i < swapChainImageViews.size(); i++) { // iterate through the image views and create framebuffers from them
			std::array<VkImageView, 2> attachments = { swapChainImageViews[i], depthImageView };

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// create the command pool to record commands for execution in their respective family
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void createCommandPool() {
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);


		// Command buffers are executed by submitting them on one of the device queues, like the graphics and presentation queues we 
		// retrieved. Each command pool can only allocate command buffers that are submitted on a single type of queue. We're going 
		// to record commands for drawing, which is why we've chosen the graphics queue family.
		//
		// There are two possible flags for command pools :
		//
		//		VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands very often(may change memory allocation behavior)
		//		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT : Allow command buffers to be rerecorded individually, without this flag they all have to be reset together
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// A uniform buffer is a buffer that is made accessible in a read-only fashion to shaders so that the shaders can read constant 
	// parameter data. This is another example of a step that you have to perform in a Vulkan program that you wouldn't have to do 
	// in another graphics API.
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void createUniformBuffers() {
		// We're going to write a separate function that updates the uniform buffer with a new transformation every frame, so there will be no vkMapMemory here.
		// The uniform data will be used for all draw calls, so the buffer containing it should only be destroyed when we stop rendering. 
		// Since it also depends on the number of swap chain images, which could change after a recreation, we'll clean it up in cleanupSwapChain
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		uniformBuffers.resize(swapChainImages.size());
		uniformBuffersMemory.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); i++) {
			createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// This function will generate a new transformation every frame based on the cameras position
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void updateUniformBuffer(uint32_t currentImage, float dt) {
		// define the model, view, and projection transformations in the uniform buffer object. 
		UniformBufferObject ubo{};
		ubo.model = glm::mat4(1.0f);
		ubo.view = camera.getViewMatrix();
		ubo.proj = camera.getProjMatrix();

		// GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted. The easiest way to 
		// compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix. If you don't do 
		// this, then the image will be rendered upside down.
		ubo.proj[1][1] *= -1;

		// All of the transformations are defined now, so we can copy the data in the uniform buffer object to the current uniform
		// buffer. This happens in exactly the same way as we did for vertex buffers, except without a staging buffer
		void* data;
		vkMapMemory(device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, uniformBuffersMemory[currentImage]);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// parameters for the buffer size, usage, and memory properties that we can use to create many different types of buffers. The 
	// last two parameters are output variables to write the handles to.
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
		// Creating a buffer requires us to fill a VkBufferCreateInfo structure. because ofc it does...
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

		// size specifies the size of the buffer in bytes.
		bufferInfo.size = size;

		// usage indicates the purposes the data in the buffer will be used for. It is possible to specify multiple purposes using a bitwise OR.
		bufferInfo.usage = usage;

		// Just like the images in the swap chain, buffers can also be owned by a specific queue family or be shared between multiple at the same time. For now, the buffer will only be used from the graphics queue, so we can stick to exclusive access.
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		// create the buffer
		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			__debugbreak();
			throw std::runtime_error("failed to create buffer!");
		}

		// query buffer memory requirements
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		// use those requirements to findMemoryType for allocation info
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		// allocate the vertex buffer memory
		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			__debugbreak();
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		// bind the memory to the buffer. The first three parameters are self-explanatory and the fourth parameter is the offset 
		// within the region of memory. Since this memory is allocated specifically for this the vertex buffer, the offset is simply
		// 0. If the offset is non-zero, then it is required to be divisible by memRequirements.alignment.
		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Descriptor sets can't be created directly, they must be allocated from a pool like command buffers. The equivalent for 
	// descriptor sets is unsurprisingly called a descriptor pool
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void createDescriptorPool() {
		// We first need to describe which descriptor types our descriptor sets are going to contain and how many of them, 
		// using VkDescriptorPoolSize structures.
		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

		// Aside from the maximum number of individual descriptors that are available, we also need to specify the maximum number 
		// of descriptor sets that may be allocated
		poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());;

		// The structure has an optional flag similar to command pools that determines if individual descriptor sets can be freed or
		// not: VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT. We're not going to touch the descriptor set after creating it, 
		// so we don't need this flag. You can leave flags to its default value of 0.


		// The descriptor pool should be destroyed when the swap chain is recreated because it depends on the number of images. see cleanupSwapChain()
		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// A descriptor set allocation is described with a VkDescriptorSetAllocateInfo struct. You need to specify the descriptor pool 
	// to allocate from, the number of descriptor sets to allocate, and the descriptor layout to base them on
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void createDescriptorSets() {
		// In our case we will create one descriptor set for each swap chain image, all with the same layout. Unfortunately we do 
		// need all the copies of the layout because the next function expects an array matching the number of sets.
		std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = (uint32_t)swapChainImages.size();
		allocInfo.pSetLayouts = layouts.data();

		descriptorSets.resize(swapChainImages.size());
		if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) { // You don't need to explicitly clean up descriptor sets, because they will be automatically freed when the descriptor pool is destroyed. The call to vkAllocateDescriptorSets will allocate descriptor sets, each with one uniform buffer descriptor.
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		// The descriptor sets have been allocated now, but the descriptors within still need to be configured. We'll now add a loop to populate every descriptor
		// Descriptors that refer to buffers, like our uniform buffer descriptor, are configured with a VkDescriptorBufferInfo struct. 
		// This structure specifies the buffer and the region within it that contains the data for the descriptor.
		for (size_t i = 0; i < swapChainImages.size(); i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = textureImageViews[0];
			imageInfo.sampler = textureSampler;

			std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// copy the contents of one buffer to another buffer
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		endSingleTimeCommands(commandBuffer);
	}

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		} else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		endSingleTimeCommands(commandBuffer);
	}

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferImageCopy imgCopy{};
		imgCopy.bufferOffset = 0;
		imgCopy.bufferRowLength = 0;
		imgCopy.bufferImageHeight = 0;

		imgCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgCopy.imageSubresource.mipLevel = 0;
		imgCopy.imageSubresource.baseArrayLayer = 0;
		imgCopy.imageSubresource.layerCount = 1;

		imgCopy.imageOffset = { 0, 0, 0 };
		imgCopy.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopy);

		endSingleTimeCommands(commandBuffer);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// combine the requirements of the vertex buffer and our own application to find the right type of memory to use
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		// First we need to query info about the available types of memory using vkGetPhysicalDeviceMemoryProperties.
		// The VkPhysicalDeviceMemoryProperties structure has two arrays memoryTypes and memoryHeaps. Memory heaps are distinct
		// memory resources like dedicated VRAM and swap space in RAM for when VRAM runs out. The different types of memory exist 
		// within these heaps. Right now we'll only concern ourselves with the type of memory and not the heap it comes from, but
		// you can imagine that this can affect performance.
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		// find a memory type that is suitable for the buffer itself.
		// The typeFilter parameter will be used to specify the bit field of memory types that are suitable. That means that we can
		// find the index of a suitable memory type by simply iterating over them and checking if the corresponding bit is set to 1.
		// but we also need to be able to write our vertex data to that memory. The memoryTypes array consists of VkMemoryType
		// structs that specify the heap and properties of each type of memory. The properties define special features of the
		// memory, like being able to map it so we can write to it from the CPU. This property is indicated with
		// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, but we also need to use the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT property. We'll see
		// why when we map the memory.
		// We may have more than one desirable property, so we should check if the result of the bitwise AND is not just non-zero, 
		// but equal to the desired properties bit field. If there is a memory type suitable for the buffer that also has all of 
		// the properties we need, then we return its index, otherwise we throw an exception.
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// create the command buffers
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void createCommandBuffers() {
		commandBuffers.resize(swapChainFramebuffers.size());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

		if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// create the semaphores and fences to use in syncronizing the drawing of frames as well as syncing the CPU and GPU
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void createSyncObjects() {
		imageAvailableSemaphores.resize(config->maxFramesInFlight);
		renderFinishedSemaphores.resize(config->maxFramesInFlight);
		inFlightFences.resize(config->maxFramesInFlight);
		imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < config->maxFramesInFlight; i++) {
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

				throw std::runtime_error("failed to create synchronization objects for a frame!");
			}
		}
	}

	void cleanupSwapChain() {
		vkDestroyImageView(device, depthImageView, nullptr);
		vkDestroyImage(device, depthImage, nullptr);
		vkFreeMemory(device, depthImageMemory, nullptr);

		for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
			vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
		}

		vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);

		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			vkDestroyImageView(device, swapChainImageViews[i], nullptr);
		}

		vkDestroySwapchainKHR(device, swapChain, nullptr);

		// uniform buffers depends on the number of images in the swapchain so it is cleaned up here. dont forget to recreate it in recreateSwapChain()
		for (size_t i = 0; i < swapChainImages.size(); i++) {
			vkDestroyBuffer(device, uniformBuffers[i], nullptr);
			vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
		}

		// The descriptor pool should be destroyed when the swap chain is recreated because it depends on the number of images. recreate it in recreateSwapChain().
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// It is possible for the window surface to change such that the swap chain is no longer compatible with it. One of the reasons 
	// that could cause this to happen is the size of the window changing. We have to catch these events and recreate the swap chain.
	// There is another case where a swap chain may become out of data and that is a special kind of window resizing: window 
	// minimization. This case is special because it will result in a frame buffer size of 0. We will handle that by pausing until 
	// the window is in the foreground again.
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void recreateSwapChain() {
		int width = 0, height = 0;
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		config->windowX = width;
		config->windowY = height;
		camera.recreateProjectionMatrix(*config);

		vkDeviceWaitIdle(device);

		cleanupSwapChain();

		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createDepthResources();
		createFrameBuffers();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createCommandBuffers();
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// create the shader module to pass into the pipeline
	//
	// Creating a shader module is simple, we only need to specify a pointer to the buffer with the bytecode and the length of it. 
	// This information is specified in a VkShaderModuleCreateInfo structure. The one catch is that the size of the bytecode is 
	// specified in bytes, but the bytecode pointer is a uint32_t pointer rather than a char pointer. Therefore we will need to cast 
	// the pointer with reinterpret_cast. When you perform a cast like this, you also need to ensure that the data satisfies the 
	// alignment requirements of uint32_t. Lucky for us, the data is stored in a std::vector where the default allocator already 
	// ensures that the data satisfies the worst case alignment requirements.
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	VkShaderModule createShaderModule(const std::vector<char>& code) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderModule;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// read all of the bytes from the specified file and return them in a byte array managed by std::vector
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary); // ate: start reading at the end of the file. binary: read the file as binary (avoid text transformations)

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file: " + filename);
		}

		// start reading at the end of the file so that we can use the read position to determine the size of the file and allocate a buffer
		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		// now, seek back to the beginning of the file and read all of the bytes at once
		file.seekg(0);
		file.read(buffer.data(), fileSize);


		file.close(); // don't forget to close the file dumbass!
		return buffer;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// this is the callback to use for the debug messenger. this prints out the actual messages.
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
		std::cerr << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}
};

void handleKeyboardInput(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_W && action == GLFW_PRESS) { controller.forwardPressed = true; }
	if (key == GLFW_KEY_S && action == GLFW_PRESS) { controller.backPressed = true; }
	if (key == GLFW_KEY_A && action == GLFW_PRESS) { controller.leftPressed = true; }
	if (key == GLFW_KEY_D && action == GLFW_PRESS) { controller.rightPressed = true; }
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) { controller.upPressed = true; }
	if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_PRESS && player.isFlying) { controller.downPressed = true; }
	if (key == GLFW_KEY_F && action == GLFW_PRESS) { controller.flyToggleNew = !controller.flyToggleNew; }
	if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS) { controller.speedModifierPressed = true; }
	

	if (key == GLFW_KEY_W && action == GLFW_RELEASE) { controller.forwardPressed = false; }
	if (key == GLFW_KEY_S && action == GLFW_RELEASE) { controller.backPressed = false; }
	if (key == GLFW_KEY_A && action == GLFW_RELEASE) { controller.leftPressed = false; }
	if (key == GLFW_KEY_D && action == GLFW_RELEASE) { controller.rightPressed = false; }
	if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE) { controller.upPressed = false; }
	if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_RELEASE && player.isFlying) { controller.downPressed = false; }
	if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_RELEASE) { controller.speedModifierPressed = false; }
	
}

double lastMouseX;
double lastMouseY;

void handleMouseInput(GLFWwindow* window, double xpos, double ypos) {
	if (firstMouse) { 
		firstMouse = false; 
		lastMouseX = xpos;
		lastMouseY = ypos;
		return;
	}

	static float const BOUND = 89.f;
	double dx = xpos - lastMouseX;
	double dy = ypos - lastMouseY;

	auto rotation = &(player.rotation);

	rotation->y += dx * sensitivity;
	rotation->x += dy * sensitivity;

	if (rotation->x > BOUND)
		rotation->x = BOUND;
	else if (rotation->x < -BOUND)
		rotation->x = -BOUND;

	if (rotation->y > 360)
		rotation->y = 0;
	else if (rotation->y < 0)
		rotation->y = 360;

	lastMouseX = xpos;
	lastMouseY = ypos;
}

void handleMouseButtonInput(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) { controller.leftClicked = true; }
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) { controller.rightClicked = true; }
}