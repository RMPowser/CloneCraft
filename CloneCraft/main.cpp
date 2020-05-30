#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// The glm/gtc/matrix_transform.hpp header exposes functions that can be used to generate model transformations such as glm::rotate,
// view transformations such as glm::lookAt, and projection transformations such as glm::perspective. The GLM_FORCE_RADIANS 
// definition is necessary to make sure that functions like glm::rotate use radians as arguments, to avoid any possible confusion.
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES // force GLM to use a version of its types that has the alignment requirements already specified for us
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION // The header only defines the prototypes of the functions by default. One code file needs to include the header with the STB_IMAGE_IMPLEMENTATION definition to include the function bodies, otherwise we'll get linking errors.
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

// The chrono standard library header exposes functions to do precise timekeeping. We'll use this to make sure that the geometry
// rotates 90 degrees per second regardless of frame rate.
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
#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#define PAUSE system("pause")


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// define globals
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;
const char* WINDOW_TITLE = "CloneCraft   :^)";
const int MAX_FRAMES_IN_FLIGHT = 2; // defines how many frames should be processed concurrently
const std::string MODEL_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "textures/viking_room.png";

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

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

// The data in the matrices is binary compatible with the way the shader expects it, so later we can just memcpy a UniformBufferObject to a VkBuffer.
struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// check debug
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

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
VkResult CreateDebugUtilsMessengerEXT(
	VkInstance                                  instance, 
	const VkDebugUtilsMessengerCreateInfoEXT*   pCreateInfo, 
	const VkAllocationCallbacks*                pAllocator, 
	VkDebugUtilsMessengerEXT*                   pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// destroys the dubug messenger 
//
// DestroyDebugUtilsMessengerEXT parameters:
//      instance: the instance where the callback was created.
//      debugMessenger: the VkDebugUtilsMessengerEXT object to destroy. messenger is an externally synchronized object and must 
//                      not be used on more than one thread at a time. This means that vkDestroyDebugUtilsMessengerEXT MUST NOT 
//                      be called when a callback is active.
//      pAllocator: (always NULL in this program) controls host memory allocation as described in the Memory Allocation chapter. https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#memory-allocation
//
// vkGetInstanceProcAddr parameters: 
//      instance: instance that the function pointer will be compatible with, or NULL for commands not dependent on any instance
//      pName: name of the command to obtain
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DestroyDebugUtilsMessengerEXT(
	VkInstance                      instance, 
	VkDebugUtilsMessengerEXT        debugMessenger, 
	const VkAllocationCallbacks*    pAllocator)
{
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
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	GLFWwindow* window;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers; // Command buffers will be automatically freed when their command pool is destroyed, so we don't need an explicit cleanup.
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	size_t currentFrame = 0; // keeps track of the current frame. no way! :D
	bool framebufferResized = false;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// create a window using glfw because vulkan cant create windows
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void initWindow() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}

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
		createTextureImage();
		createTextureImageView();
		createTextureSampler();
		loadModel();
		createVertexBuffer();
		createIndexBuffer();
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
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			drawFrame();
		}

		vkDeviceWaitIdle(device); // when exiting the loop, drawing and presentation operations may still be going on. Cleaning up resources while that is happening is a bad idea.
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// clean up any memory in reverse order of declaration
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void cleanup() { 
		cleanupSwapChain();

		vkDestroySampler(device, textureSampler, nullptr);
		vkDestroyImageView(device, textureImageView, nullptr);

		vkDestroyImage(device, textureImage, nullptr);
		vkFreeMemory(device, textureImageMemory, nullptr);

		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vkDestroyBuffer(device, indexBuffer, nullptr);
		vkFreeMemory(device, indexBufferMemory, nullptr);

		vkDestroyBuffer(device, vertexBuffer, nullptr);
		vkFreeMemory(device, vertexBufferMemory, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}

		vkDestroyCommandPool(device, commandPool, nullptr);

		vkDestroyDevice(device, nullptr);

		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}

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
			if (enableValidationLayers && !checkValidationLayerSupport()) {
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

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		} else {
			createInfo.enabledLayerCount = 0;

			createInfo.pNext = nullptr;
		}

		try {
			if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
				throw std::runtime_error("failed to create vulkan instance\n");
			}
		} catch (std::runtime_error& e) {
			std::cerr << e.what();
			std::terminate();
		}
	}

	void loadModel() {
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
		if (!enableValidationLayers) return;

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
		
		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

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

		std::cout << "available validation layers:\n";
		for (const char* layerName : validationLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					std::cout << "\t" << layerProperties.layerName << " v" << layerProperties.implementationVersion << std::endl;
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
			throw std::runtime_error("failed to fund GPU with Vulkan support");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		for (const auto& device : devices) {
			if (isDeviceSuitable(device)) {
				physicalDevice = device;
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
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		} else {
			createInfo.enabledLayerCount = 0;
		}

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

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

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
	// create the swap chain using all of the helper functions 
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void createSwapChain() {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		// 1/2 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Aside from the properties above we also have to decide how many images we would like to have in the swap chain. The 
		// implementation specifies the minimum number that it requires to function:
		// uint32_t imageCount = swapChainSupport.capabilities.minImageCount;
		// However, simply sticking to this minimum means that we may sometimes have to wait on the driver to complete internal 
		// operations before we can acquire another image to render to. Therefore it is recommended to request at least one more 
		// image than the minimum:
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		// 2/2 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// We should also make sure to not exceed the maximum number of images while doing this, where 0 is a special value that 
		// means that there is no maximum:
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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


		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Next, we need to specify how to handle swap chain images that will be used across multiple queue families. This will be 
		// the case if the graphics queue family is different from the presentation queue. We'll be drawing on the images in the swap
		// chain from the graphics queue and then submitting them on the presentation queue.
		// 
		// There are two ways to handle images that are accessed from multiple queues:
		//      VK_SHARING_MODE_EXCLUSIVE: An image is owned by one queue family at a time and ownership must be explicitly 
		//          transfered before using it in another queue family.This option offers the best performance.
		// 
		//      VK_SHARING_MODE_CONCURRENT: Images can be used across multiple queue families without explicit ownership transfers.
		//
		// If the queue families differ, then we'll be using the concurrent mode to avoid having to deal with ownership. Concurrent 
		// mode requires you to specify in advance between which queue families ownership will be shared using the 
		// queueFamilyIndexCount and pQueueFamilyIndices parameters. If the graphics queue family and presentation queue family are 
		// the same, which will be the case on most hardware, then we should stick to exclusive mode, because concurrent mode 
		// requires you to specify at least two distinct queue families.
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// We can specify that a certain transform should be applied to images in the swap chain if it is supported 
		// (supportedTransforms in capabilities), like a 90 degree clockwise rotation or horizontal flip. To specify that you do not
		// want any transformation, simply specify the current transformation.
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;


		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //The compositeAlpha field specifies if the alpha channel should be used for blending with other windows in the window system. You'll almost always want to simply ignore the alpha channel, hence VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR.
		
		// The presentMode member speaks for itself. If the clipped member is set to VK_TRUE then that means that we don't care about the color of pixels that are obscured, for example because another window is in front of them. Unless you really need to be able to read these pixels back and get predictable results, you'll get the best performance by enabling clipping.
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		// That leaves one last field, oldSwapChain. With Vulkan it's possible that your swap chain becomes invalid or unoptimized while your application is running, for example because the window was resized. In that case the swap chain actually needs to be recreated from scratch and a reference to the old one must be specified in this field. This is a complex topic that we'll learn more about in a future chapter. For now we'll assume that we'll only ever create one swap chain.
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		// create the swapchain. ezpz
		// params: (logical device, swap chain createinfo, pAllocator always nullptr in this program, somewhere to store the swap chain)
		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}

		// Retrieving swapchain images is very similar to the other times where we retrieved an array of objects from Vulkan. Remember that we only specified a MINIMUM number of images in the swap chain, so the implementation is allowed to create a swap chain with more. That's why we'll first query the final number of images with vkGetSwapchainImagesKHR, then resize the container and finally call it again to retrieve the handles.
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

		// need these later
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

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// choose a presentation mode preferring VK_PRESENT_MODE_MAILBOX_KHR for triple buffering
	//
	// There are four possible modes available in Vulkan:
	//      VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted by your application are transferred to the screen right away, which may 
	//          result in tearing.
	//
	//      VK_PRESENT_MODE_FIFO_KHR: The swap chain is a queue where the display takes an image from the front of the queue when 
	//          the display is refreshed and the program inserts rendered images at the back of the queue. If the queue is full then
	//          the program has to wait. This is most similar to vertical sync as found in modern games. The moment that the display
	//          is refreshed is known as "vertical blank".
	//
	//      VK_PRESENT_MODE_FIFO_RELAXED_KHR: This mode only differs from the previous one if the application is late and the queue 
	//          was empty at the last vertical blank. Instead of waiting for the next vertical blank, the image is transferred right 
	//          away when it finally arrives. This may result in visible tearing.
	//
	//      VK_PRESENT_MODE_MAILBOX_KHR: This is another variation of the second mode. Instead of blocking the application when the 
	//          queue is full, the images that are already queued are simply replaced with the newer ones. This mode can be used to 
	//          implement triple buffering, which allows you to avoid tearing with significantly less latency issues than standard 
	//          vertical sync that uses double buffering.
	//
	// Only the VK_PRESENT_MODE_FIFO_KHR mode is guaranteed to be available.
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The swap extent is the resolution of the swap chain images and it's almost always exactly equal to the resolution of the 
	// window that we're drawing to. The range of the possible resolutions is defined in the VkSurfaceCapabilitiesKHR structure. 
	// Vulkan tells us to match the resolution of the window by setting the width and height in the currentExtent member. However, 
	// some window managers do allow us to differ here and this is indicated by setting the width and height in currentExtent to a 
	// special value: the maximum value of uint32_t. In that case we'll pick the resolution that best matches the window within the 
	// minImageExtent and maxImageExtent bounds.
	// The max and min functions are used here to clamp the value of width and height between the allowed minimum and 
	// maximum extents that are supported by the implementation.
	// To handle window resizes properly, we also need to query the current size of the framebuffer to make sure that the swap chain 
	// images have the (new) right size.
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
		// attachment description
		// In our case we'll have just a single color buffer attachment represented by one of the images from the swap chain.
		// The format of the color attachment should match the format of the swap chain images, and we're not doing anything with multisampling yet, so we'll stick to 1 sample.
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

		// The loadOp and storeOp determine what to do with the data in the attachment before rendering and after rendering. 
		// We have the following choices for loadOp:
		//
		//		VK_ATTACHMENT_LOAD_OP_LOAD: Preserve the existing contents of the attachment
		//		VK_ATTACHMENT_LOAD_OP_CLEAR : Clear the values to a constant at the start
		//		VK_ATTACHMENT_LOAD_OP_DONT_CARE : Existing contents are undefined; we don't care about them
		//
		// In our case we're going to use the clear operation to clear the framebuffer to black before drawing a new frame. 
		// There are only two possibilities for the storeOp:
		//
		//		VK_ATTACHMENT_STORE_OP_STORE: Rendered contents will be stored in memory and can be read later
		//		VK_ATTACHMENT_STORE_OP_DONT_CARE : Contents of the framebuffer will be undefined after the rendering operation
		//
		// We're interested in seeing the rendered triangle on the screen, so we're going with the store operation.
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		// The loadOp and storeOp apply to color and depth data, and stencilLoadOp/stencilStoreOp apply to stencil data. This 
		// application won't do anything with the stencil buffer, so the results of loading and storing are irrelevant.
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		// Textures and framebuffers in Vulkan are represented by VkImage objects with a certain pixel format, however the layout of
		// the pixels in memory can change based on what you're trying to do with an image.
		// Some of the most common layouts are:
		//
		//		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
		//		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : Images to be presented in the swap chain
		//		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : Images to be used as destination for a memory copy operation
		//
		// What's important to know right now is that images need to be transitioned to specific layouts that are suitable for the
		// operation that they're going to be involved in next. The initialLayout specifies which layout the image will have before
		// the render pass begins. The finalLayout specifies the layout to automatically transition to when the render pass finishes.
		// Using VK_IMAGE_LAYOUT_UNDEFINED for initialLayout means that we don't care what previous layout the image was in. The 
		// caveat of this special value is that the contents of the image are not guaranteed to be preserved, but that doesn't matter
		// since we're going to clear it anyway. We want the image to be ready for presentation using the swap chain after rendering,
		// which is why we use VK_IMAGE_LAYOUT_PRESENT_SRC_KHR as finalLayout.
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

		// Subpasses and attachment references
		// A single render pass can consist of multiple subpasses. Subpasses are subsequent rendering operations that depend on the 
		// contents of framebuffers in previous passes, for example a sequence of post-processing effects that are applied one after
		// another. If you group these rendering operations into one render pass, then Vulkan is able to reorder the operations and 
		// conserve memory bandwidth for possibly better performance. For our very first triangle, however, we'll stick to a single subpass.
		// Every subpass references one or more of the attachments that we've described using the structure in the previous sections.
		//
		// The attachment parameter specifies which attachment to reference by its index in the attachment descriptions array. Our 
		// array consists of a single VkAttachmentDescription, so its index is 0. The layout specifies which layout we would like 
		// the attachment to have during a subpass that uses this reference. Vulkan will automatically transition the attachment to 
		// this layout when the subpass is started. We intend to use the attachment to function as a color buffer and the 
		// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL layout will give us the best performance, as its name implies.
		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// The subpass is described using a VkSubpassDescription structure. Vulkan also supports compute subpasses, so we have to be
		// explicit about this being a graphics subpass.
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		// specify the reference to the color attachment 
		// The index of the attachment in this array is directly referenced from the fragment shader with the layout(location = 0) 
		// out vec4 outColor directive!
		// The following other types of attachments can be referenced by a subpass:
		//
		//		pInputAttachments: Attachments that are read from a shader
		//		pResolveAttachments : Attachments used for multisampling color attachments
		//		pDepthStencilAttachment : Attachment for depthand stencil data
		//		pPreserveAttachments : Attachments that are not used by this subpass, but for which the data must be preserved
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		// Remember that the subpasses in a render pass automatically take care of image layout transitions. These transitions 
		// are controlled by subpass dependencies, which specify memory and execution dependencies between subpasses. We have 
		// only a single subpass right now, but the operations right before and right after this subpass also count as implicit
		// "subpasses". There are two built-in dependencies that take care of the transition at the start of the render pass 
		// and at the end of the render pass, but the former does not occur at the right time. It assumes that the transition 
		// occurs at the start of the pipeline, but we haven't acquired the image yet at that point! There are two ways to deal
		// with this problem. We could change the waitStages for the imageAvailableSemaphore to VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
		// to ensure that the render passes don't begin until the image is available, or we can make the render pass wait for 
		// the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage. I've decided to go with the second option here, because it's
		// a good excuse to have a look at subpass dependencies and how they work.
		// 
		// The first two fields specify the indices of the dependency and the dependent subpass. The special value 
		// VK_SUBPASS_EXTERNAL refers to the implicit subpass before or after the render pass depending on whether it is 
		// specified in srcSubpass or dstSubpass. The index 0 refers to our subpass, which is the first and only one. The 
		// dstSubpass must always be higher than srcSubpass to prevent cycles in the dependency graph 
		// (unless one of the subpasses is VK_SUBPASS_EXTERNAL).
		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;

		// The next two fields specify the operations to wait on and the stages in which these operations occur. We need to wait
		// for the swap chain to finish reading from the image before we can access it. This can be accomplished by waiting on 
		// the color attachment output stage itself.
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;

		// The operations that should wait on this are in the color attachment stage and involve the writing of the color 
		// attachment. These settings will prevent the transition from happening until it's actually necessary (and allowed) which
		// is when we want to start writing colors to it.
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

		// render pass
		// The render pass object can be created by filling in the VkRenderPassCreateInfo structure with an array of attachments
		// and subpasses. The VkAttachmentReference objects reference attachments using the indices of this array.
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		// The VkRenderPassCreateInfo struct has two fields to specify an array of dependencies.
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;



		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}

	void createTextureImage() {
		// The stbi_load function takes the file path and number of channels to load as arguments. 
		// The STBI_rgb_alpha value forces the image to be loaded with an alpha channel, even if it 
		// doesn't have one, which is nice for consistency with other textures in the future. The 
		// middle three parameters are outputs for the width, height and actual number of channels in the image. 
		// The pointer that is returned is the first element in an array of pixel values. The pixels are 
		// laid out row by row with 4 bytes per pixel in the case of STBI_rgb_alpha for a total of texWidth * texHeight * 4 values.
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}

		// create a buffer in host visible memory so that we can use vkMapMemory and copy the pixels to it
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		// The buffer should be in host visible memory so that we can map it and it should be usable as a transfer source so that we can copy it to an image later on
		createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
		
		// We can then directly copy the pixel values that we got from the image loading library to the buffer
		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(device, stagingBufferMemory);

		// dont forget to clean up the original pixel array now
		stbi_image_free(pixels);

		createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
		
		transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// The image was created with the VK_IMAGE_LAYOUT_UNDEFINED layout, so that one should be specified as old layout when transitioning textureImage. Remember that we can do this because we don't care about its contents before performing the copy operation.
		copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

		// To be able to start sampling from the texture image in the shader, we need one last transition to prepare it for shader access
		transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void createTextureImageView() {
		textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
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
	// create the graphics pipeline
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
		rasterizer.cullMode = VK_CULL_MODE_NONE;
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
		colorBlendAttachment.blendEnable = VK_FALSE;

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
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
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
		//
		// We will only record the command buffers at the beginning of the program and then execute them many times in the main loop, so we're not going to use either of these flags.
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		poolInfo.flags = 0; // Optional

		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// create the vertex buffer used to pass vertices into gpu memory
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void createVertexBuffer() {
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();


		// We're now using a new stagingBuffer with stagingBufferMemory for mapping and copying the vertex data. 
		// We're going to use two new buffer usage flags :
		//
		//		VK_BUFFER_USAGE_TRANSFER_SRC_BIT: Buffer can be used as source in a memory transfer operation.
		//		VK_BUFFER_USAGE_TRANSFER_DST_BIT: Buffer can be used as destination in a memory transfer operation.
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		// see createBuffer() for specifics on how buffer creation works
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		// we need to copy the vertex data to the buffer. This is done by mapping the buffer memory into CPU accessible memory
		// with vkMapMemory.
		// This function allows us to access a region of the specified memory resource defined by an offset and size. The offset and 
		// size here are 0 and bufferInfo.size, respectively. It is also possible to specify the special value VK_WHOLE_SIZE to map
		// all of the memory. The second to last parameter can be used to specify flags, but there aren't any available yet in the 
		// current API (1.2) and it is reserved for future use. It must be set to the value 0. The last parameter specifies the
		// output for the pointer to the mapped memory.
		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);

		// You can now simply memcpy the vertex data to the mapped memory and unmap it again using vkUnmapMemory.
		memcpy(data, vertices.data(), (size_t) bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		// Unfortunately the driver may not immediately copy the data into the buffer memory, for example because of caching. It is 
		// also possible that writes to the buffer are not visible in the mapped memory yet. There are two ways to deal with that problem:
		//
		//		Use a memory heap that is host coherent, indicated with VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		//		Call vkFlushMappedMemoryRanges after writing to the mapped memory, and call vkInvalidateMappedMemoryRanges before reading from the mapped memory
		//
		// We went for the first approach, which ensures that the mapped memory always matches the contents of the allocated memory. 
		// Do keep in mind that this may lead to slightly worse performance than explicit flushing, but we'll see why that doesn't matter later.
		// 
		// Flushing memory ranges or using a coherent memory heap means that the driver will be aware of our writes to the buffer, 
		// but it doesn't mean that they are actually visible on the GPU yet. The transfer of data to the GPU is an operation that 
		// happens in the background and the specification simply tells us that it is guaranteed to be complete as of the next call to vkQueueSubmit.


		// The vertexBuffer is now allocated from a memory type that is device local, which generally means that we're not able to 
		// use vkMapMemory. However, we can copy data from the stagingBuffer to the vertexBuffer. We have to indicate that we intend
		// to do that by specifying the transfer source flag for the stagingBuffer and the transfer destination flag for the 
		// vertexBuffer, along with the vertex buffer usage flag.
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
		
		// use copyBuffer() to move the vertex data to the device local buffer
		copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
		
		// After copying the data from the staging buffer to the device buffer, we should clean it up
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// almost identical to createVertexBuffer(). only two notable differences. bufferSize is now equal to the number of indices times
	// the size of the index type, either uint16_t or uint32_t. The usage of the indexBuffer should be 
	// VK_BUFFER_USAGE_INDEX_BUFFER_BIT instead of VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, which makes sense. Other than that, the process
	// is exactly the same. We create a staging buffer to copy the contents of the indices array to and then copy it to the final 
	// device local index buffer.
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void createIndexBuffer() {
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

		copyBuffer(stagingBuffer, indexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
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
	// This function will generate a new transformation every frame to make the geometry spin around.
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void updateUniformBuffer(uint32_t currentImage) {
		// start out with some logic to calculate the time in seconds since rendering has started with floating point accuracy.
		static auto startTime	= std::chrono::high_resolution_clock::now();
		auto currentTime		= std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		// now define the model, view, and projection transformations in the uniform buffer object. The model rotation will be a 
		// simple rotation around the Z-axis using the time variable
		// The glm::rotate function takes an existing transformation, rotation angle and rotation axis as parameters. The 
		// glm::mat4(1.0f) constructor returns an identity matrix. Using a rotation angle of time * glm::radians(90.0f) accomplishes
		// the rotation of 90 degrees per second.
		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		// For the view transformation I've decided to look at the geometry from above at a 45 degree angle. The glm::lookAt function
		// takes the eye position, center position and up axis as parameters.
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		// I've chosen to use a perspective projection with a 45 degree vertical field-of-view. The other parameters are the aspect 
		// ratio, near and far view planes. It is important to use the current swap chain extent to calculate the aspect ratio to 
		// take into account the new width and height of the window after a resize.
		ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);

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

		// usage indicates the purposes the data in the buffer will be used for. It is possible to specify multiple purposes using a bitwise or.
		bufferInfo.usage = usage;

		// Just like the images in the swap chain, buffers can also be owned by a specific queue family or be shared between multiple at the same time. For now, the buffer will only be used from the graphics queue, so we can stick to exclusive access.
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		// create the buffer
		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
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
			imageInfo.imageView = textureImageView;
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

		for (size_t i = 0; i < commandBuffers.size(); i++) {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
				throw std::runtime_error("failed to begin recording command buffer!");
			}

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = swapChainFramebuffers[i];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = swapChainExtent;

			std::array<VkClearValue, 2> clearValues{};
			clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
			clearValues[1].depthStencil = { 1.0f, 0 };

			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

				vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

				VkBuffer vertexBuffers[] = { vertexBuffer };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

				vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

				vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

				vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

			vkCmdEndRenderPass(commandBuffers[i]);

			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Draw the god damned frame
	// function will perform the following operations:
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
	void drawFrame() {
		// Wait for the previous frame to be finished. The vkWaitForFences function takes an array of fences and waits for either 
		// any or all of them to be signaled before returning. The VK_TRUE we pass here indicates that we want to wait for all fences,
		// but in the case of a single one it obviously doesn't matter. Just like vkAcquireNextImageKHR this function also takes a 
		// timeout. Unlike the semaphores, we manually need to restore the fence to the unsignaled state by resetting it with the 
		// vkResetFences call.
		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
		

		// We'll need one semaphore to signal that an image has been acquired and is ready for rendering, and another one to signal
		// that rendering has finished and presentation can happen. These are created in createSemaphores() and are stored in the 
		// following class members:
		//		std::vector<VkSemaphore> imageAvailableSemaphores;
		//		std::vector<VkSemaphore> renderFinishedSemaphores;	

		// As mentioned before, the first thing we need to do is acquire an image from the swap chain. Recall that the swap chain is
		// an extension feature, so we must use a function with the vk*KHR naming convention. The first two parameters of 
		// vkAcquireNextImageKHR are the logical device and the swap chain from which we wish to acquire an image. The third 
		// parameter specifies a timeout in nanoseconds for an image to become available. Using the maximum value of a 64 bit 
		// unsigned integer disables the timeout. The next two parameters specify synchronization objects that are to be signaled 
		// when the presentation engine is finished using the image. That's the point in time where we can start drawing to it. It 
		// is possible to specify a semaphore, fence or both. We're going to use our imageAvailableSemaphore for that purpose here.
		// The last parameter specifies a variable to output the index of the swap chain image that has become available. The index
		// refers to the VkImage in our swapChainImages array. We're going to use that index to pick the right command buffer.
		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		// now we know which swap chain image we're going to aquire. update the uniform buffer with it.
		updateUniformBuffer(imageIndex);

		// If the swap chain turns out to be out of date when attempting to acquire an image, then it is no longer possible to present
		// to it. Therefore we should immediately recreate the swap chain and try again in the next drawFrame call.
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

		// Queue submission and synchronization is configured through parameters in the VkSubmitInfo structure. The first three 
		// parameters specify which semaphores to wait on before execution begins and in which stage(s) of the pipeline to wait. We
		// want to wait with writing colors to the image until it's available, so we're specifying the stage of the graphics pipeline
		// that writes to the color attachment. That means that theoretically the implementation can already start executing our 
		// vertex shader and such while the image is not yet available. Each entry in the waitStages array corresponds to the 
		// semaphore with the same index in pWaitSemaphores.
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		// The next two parameters specify which command buffers to actually submit for execution. As mentioned earlier, we should 
		// submit the command buffer that binds the swap chain image we just acquired as color attachment.
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

		// The signalSemaphoreCount and pSignalSemaphores parameters specify which semaphores to signal once the command buffer(s) 
		// have finished execution. In our case we're using the renderFinishedSemaphore for that purpose.
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		// We can now submit the command buffer to the graphics queue using vkQueueSubmit. The function takes an array of 
		// VkSubmitInfo structures as argument for efficiency when the workload is much larger. The last parameter is an optional 
		// parameter to pass a fence that should be signaled when the command buffer finishes executing. We can use this to signal 
		// that a frame has finished.
		vkResetFences(device, 1, &inFlightFences[currentFrame]);

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		// The last step of drawing a frame is submitting the result back to the swap chain to have it eventually show up on the 
		// screen. Presentation is configured through a VkPresentInfoKHR structure.
		// 
		// The first two parameters specify which semaphores to wait on before presentation can happen, just like VkSubmitInfo.
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		// The next two parameters specify the swap chains to present images to and the index of the image for each swap chain. 
		// This will almost always be a single one.
		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;

		// There is one last optional parameter called pResults. It allows you to specify an array of VkResult values to check for 
		// every individual swap chain if presentation was successful. It's not necessary if you're only using a single swap chain, 
		// because you can simply use the return value of the present function.
		presentInfo.pResults = nullptr; // Optional

		// The vkQueuePresentKHR function submits the request to present an image to the swap chain.
		result = vkQueuePresentKHR(presentQueue, &presentInfo);

		// In this case we will also recreate the swap chain if it is suboptimal, because we want the best possible result.
		// It is important to check if the framebufferwasResized after vkQueuePresentKHR to ensure that the semaphores are in a 
		// consistent state, otherwise a signalled semaphore may never be properly waited upon. 
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
			framebufferResized = false;
			recreateSwapChain();
		} else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		// Increment the frame. By using the modulo(%) operator, we ensure that the frame index loops around after every MAX_FRAMES_IN_FLIGHT enqueued frames.
		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}


	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// create the semaphores (general name for a mutex) and fences to use in syncronizing the drawing of frames as well as syncing the CPU and GPU
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void createSyncObjects() {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
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


int main() {
	CloneCraftApplication app;

	try {
		app.run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}