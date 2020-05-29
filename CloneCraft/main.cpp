#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// The glm/gtc/matrix_transform.hpp header exposes functions that can be used to generate model transformations such as glm::rotate,
// view transformations such as glm::lookAt, and projection transformations such as glm::perspective. The GLM_FORCE_RADIANS 
// definition is necessary to make sure that functions like glm::rotate use radians as arguments, to avoid any possible confusion.
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES // force GLM to use a version of its types that has the alignment requirements already specified for us
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

#define PAUSE system("pause")


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// define globals
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;
const char* WINDOW_TITLE = "CloneCraft   :^)";
const int MAX_FRAMES_IN_FLIGHT = 2; // defines how many frames should be processed concurrently

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
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;

	// A vertex binding describes at which rate to load data from memory throughout the vertices. It specifies the number of bytes between data entries and whether to move to the next data entry after each vertex or after each instance.
	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};

		// All of our per-vertex data is packed together in one array, so we're only going to have one binding. The binding parameter specifies the index of the binding in the array of bindings.
		bindingDescription.binding = 0;

		// The stride parameter specifies the number of bytes from one entry to the next
		bindingDescription.stride = sizeof(Vertex);

		// The inputRate parameter can have one of the following values:
		//
		//		VK_VERTEX_INPUT_RATE_VERTEX: Move to the next data entry after each vertex
		//		VK_VERTEX_INPUT_RATE_INSTANCE: Move to the next data entry after each instance
		//
		// We're not going to use instanced rendering, so we'll stick to per-vertex data.
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	// An attribute description struct describes how to extract a vertex attribute from a chunk of vertex data originating from a binding description. We have two attributes, position and color, so we need two attribute description structs.
	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		// The binding parameter tells Vulkan which binding the per-vertex data comes from
		attributeDescriptions[0].binding = 0;

		// The location parameter references the location directive of the input within the vertex shader
		// The input in the vertex shader with location 0 is the position, which has two 32-bit float components.
		attributeDescriptions[0].location = 0;

		// The format parameter describes the type of data for the attribute. A bit confusingly, 
		// the formats are specified using the same enumeration as color formats. The following
		// shader types and formats are commonly used together:
		//
		//		float: VK_FORMAT_R32_SFLOAT
		//		vec2 : VK_FORMAT_R32G32_SFLOAT
		//		vec3 : VK_FORMAT_R32G32B32_SFLOAT
		//		vec4 : VK_FORMAT_R32G32B32A32_SFLOAT
		//
		// You should use the format where the amount of color channels matches the number of 
		// components in the shader data type. It is allowed to use more channels than the number
		// of components in the shader, but they will be silently discarded. If the number of channels
		// is lower than the number of components, then the BGA components will use default values 
		// of (0, 0, 1). The color type (SFLOAT, UINT, SINT) and bit width should also match the type 
		// of the shader input. See the following examples:
		//
		//		ivec2: VK_FORMAT_R32G32_SINT, a 2-component vector of 32-bit signed integers
		//		uvec4: VK_FORMAT_R32G32B32A32_UINT, a 4-component vector of 32-bit unsigned integers
		//		double: VK_FORMAT_R64_SFLOAT, a double-precision(64-bit) float
		// 
		// The format parameter implicitly defines the byte size of attribute data and the offset
		// parameter specifies the number of bytes since the start of the per-vertex data to read
		// from. The binding is loading one Vertex at a time and the position attribute (pos) is
		// at an offset of 0 bytes from the beginning of this struct. This is automatically 
		// calculated using the offsetof macro.
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);


		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
};

// using exactly the same position and color values as before (within the vertex shader), but now they're combined into one array of vertices. This is known as interleaving vertex attributes.
const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

// array indices to represent the contents of the index buffer. each index corresponds to a vertex within vertices array. uint16_t because theres less that 65535 unique vertices
const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
};

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

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

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
		createFrameBuffers();
		createCommandPool();
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

		return indices.isComplete() && extensionsSupported && swapChainAdequate;
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
	//
	// To use any VkImage, including those in the swap chain, in the render pipeline we have to create a VkImageView object. An image
	// view is literally a view into an image. It describes how to access the image and which part of the image to access, for 
	// example if it should be treated as a 2D texture depth texture without any mipmapping levels.
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void createImageViews() {
		swapChainImageViews.resize(swapChainImages.size()); // The first thing we need to do is resize the list to fit all of the image views we'll be creating.

		for (size_t i = 0; i < swapChainImages.size(); i++) { // iterate over all the swap chain images
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];

			// The viewType and format fields specify how the image data should be interpreted. The viewType parameter allows you to treat images as 1D textures, 2D textures, 3D textures and cube maps.
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;

			// The components field allows you to swizzle the color channels around. For example, you can map all of the channels to the red channel for a monochrome texture. You can also map constant values of 0 and 1 to a channel. In our case we'll stick to the default mapping.
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			// The subresourceRange field describes what the image's purpose is and which part of the image should be accessed. Our images will be used as color targets without any mipmapping levels or multiple layers. If you were working on a stereographic 3D application, then you would create a swap chain with multiple layers. You could then create multiple image views for each image representing the views for the left and right eyes by accessing different layers.
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			// create the image view. ezpz. don't forget to destroy!
			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create image views!");
			}
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

		// render pass
		// The render pass object can be created by filling in the VkRenderPassCreateInfo structure with an array of attachments
		// and subpasses. The VkAttachmentReference objects reference attachments using the indices of this array.
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		// The VkRenderPassCreateInfo struct has two fields to specify an array of dependencies.
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;



		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}

	void createDescriptorSetLayout() {
		// Every binding needs to be described through a VkDescriptorSetLayoutBinding struct ofc.
		// The first two fields specify the binding used in the shader and the type of descriptor, which is a uniform buffer object.
		// It is possible for the shader variable to represent an array of uniform buffer objects, and descriptorCount specifies the
		// number of values in the array. This could be used to specify a transformation for each of the bones in a skeleton for 
		// skeletal animation, for example. Our MVP transformation is in a single uniform buffer object, so we're using a 
		// descriptorCount of 1.
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;

		// We also need to specify in which shader stages the descriptor is going to be referenced. The stageFlags field can be a 
		// combination of VkShaderStageFlagBits values or the value VK_SHADER_STAGE_ALL_GRAPHICS. In our case, we're only referencing
		// the descriptor from the vertex shader.
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		// The pImmutableSamplers field is only relevant for image sampling related descriptors. leave this to its default value.
		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		// We can create it using vkCreateDescriptorSetLayout. This function accepts a simple VkDescriptorSetLayoutCreateInfo with the array of bindings:
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &uboLayoutBinding;

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}
		
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// create the graphics pipeline
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void createGraphicsPipeline() {
		auto vertShaderCode = readFile("shaders/vert.spv");
		auto fragShaderCode = readFile("shaders/frag.spv");

		// Shader modules are just a thin wrapper around the shader bytecode that we've previously loaded from a file and the 
		// functions defined in it. The compilation and linking of the SPIR-V bytecode to machine code for execution by the GPU 
		// doesn't happen until the graphics pipeline is created. That means that we're allowed to destroy the shader modules again
		// as soon as pipeline creation is finished, which is why they are local variables in this function instead of class members
		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);


		// To actually use the shaders you need to assign them to a specific pipeline stage through VkPipelineShaderStageCreateInfo 
		// structures as part of the actual pipeline creation process. Start by filling in the structure for the vertex shader.
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

		// Telling Vulkan which pipeline stage to use the shader in. There is an enum value for each of the programmable stages of the pipeline.
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; 

		// The next two members specify the shader module containing the code, and the function to invoke, known as the entrypoint.
		// That means that it's possible to combine multiple fragment shaders into a single shader module and use different entry 
		// points to differentiate between their behaviors. However, in this case we'll stick to the standard "main".
		vertShaderStageInfo.module = vertShaderModule; // shader module containing the code
		vertShaderStageInfo.pName = "main"; // entrypoint


		// do the same for the fragment shader
		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";
		
		// definine an array that contains these two structs, which will be used to reference them in the actual pipeline creation step
		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Vertex input
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// There is no vertex data to load at the moment because its all hardcoded within the shaders. The pVertexBindingDescriptions 
		// and pVertexAttributeDescriptions members point to an array of structs that describe the aforementioned details for loading vertex data.
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Input assembly
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// The VkPipelineInputAssemblyStateCreateInfo struct describes two things: what kind of geometry will be drawn from the 
		// verticesand if primitive restart should be enabled. The former is specified in the topology member and can have values like:
		//		VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices
		//		VK_PRIMITIVE_TOPOLOGY_LINE_LIST : line from every 2 vertices without reuse
		//		VK_PRIMITIVE_TOPOLOGY_LINE_STRIP : the end vertex of every line is used as start vertex for the next line
		//		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : triangle from every 3 vertices without reuse
		//		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP : the second and third vertex of every triangle are used as first two vertices of the next triangle
		// Normally, the vertices are loaded from the vertex buffer by index in sequential order, but with an element buffer you can 
		// specify the indices to use yourself. This allows you to perform optimizations like reusing vertices. If you set the 
		// primitiveRestartEnable member to VK_TRUE, then it's possible to break up lines and triangles in the _STRIP topology modes 
		// by using a special index of 0xFFFF or 0xFFFFFFFF.
		// We intend to draw triangles, so we'll stick to the following data for the structure:
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Viewports and scissors
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// A viewport basically describes the region of the framebuffer that the output will be rendered to. This will almost 
		// always be(0, 0) to (WINDOW_WIDTH, WINDOW_HEIGHT) and in this program that will also be the case.
		// Remember that the size of the swap chain and its images may differ from the WIDTH and HEIGHT of the window. The swap chain 
		// images will be used as framebuffers later on, so we should stick to their size.
		// The minDepth and maxDepth values specify the range of depth values to use for the framebuffer. These values must be within
		// the [0.0f, 1.0f] range, but minDepth may be higher than maxDepth. If you aren't doing anything special, then you should
		// stick to the standard values of 0.0f and 1.0f.
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		// While viewports define the transformation from the image to the framebuffer, scissor rectangles define in which regions 
		// pixels will actually be stored. Any pixels outside the scissor rectangles will be discarded by the rasterizer. They 
		// function like a filter rather than a transformation. We simply want to draw to the entire framebuffer, so we'll specify
		// a scissor rectangle that covers it entirely:
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		// The viewport and scissor rectangle need to be combined into a viewport state using the VkPipelineViewportStateCreateInfo 
		// struct. It is possible to use multiple viewports and scissor rectangles on some graphics cards, so the struct's members reference
		// an array of them. Using multiple requires enabling a GPU feature (see logical device creation).
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Rasterizer
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// The rasterizer takes the geometry that is shaped by the vertices from the vertex shader and turns it into fragments to be 
		// colored by the fragment shader. It also performs depth testing, face culling and the scissor test, and it can be configured
		// to output fragments that fill entire polygons or just the edges (wireframe rendering). All this is configured using the 
		// VkPipelineRasterizationStateCreateInfo structure.
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE; // If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far planes are clamped to them as opposed to discarding them. This is useful in some special cases like shadow maps. Using this requires enabling a GPU feature.
		rasterizer.rasterizerDiscardEnable = VK_FALSE; // If rasterizerDiscardEnable is set to VK_TRUE, then geometry never passes through the rasterizer stage. This basically disables any output to the framebuffer.

		// The polygonMode determines how fragments are generated for geometry. The following modes are available:
		//		VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments
		//		VK_POLYGON_MODE_LINE : polygon edges are drawn as lines
		//		VK_POLYGON_MODE_POINT : polygon vertices are drawn as points
		// Using any mode other than fill requires enabling a GPU feature.
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL; 

		rasterizer.lineWidth = 1.0f; // Describes the thickness of lines in terms of number of fragments. The maximum line width that is supported depends on the hardware and any line thicker than 1.0f requires you to enable the wideLines GPU feature.

		// The cullMode variable determines the type of face culling to use. You can disable culling, cull the front faces, cull the
		// back faces or both. The frontFace variable specifies the vertex order for faces to be considered front-facing and can be
		// clockwise or counterclockwise.
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		// The rasterizer can alter the depth values by adding a constant value or biasing them based on a fragment's slope. This is
		// sometimes used for shadow mapping, but we won't be using it.
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Multisampling
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// The VkPipelineMultisampleStateCreateInfo struct configures multisampling, which is one of the ways to perform 
		// anti-aliasing. It works by combining the fragment shader results of multiple polygons that rasterize to the same pixel. 
		// This mainly occurs along edges, which is also where the most noticeable aliasing artifacts occur. Because it doesn't need
		// to run the fragment shader multiple times if only one polygon maps to a pixel, it is significantly less expensive than 
		// simply rendering to a higher resolution and then downscaling. Enabling it requires enabling a GPU feature.
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Depth and stencil testing
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// If you are using a depth and/or stencil buffer, then you also need to configure the depth and stencil tests using 
		// VkPipelineDepthStencilStateCreateInfo. We don't have one right now, so we can simply pass a nullptr instead of a pointer 
		// to such a struct. Pass nullptr by default.

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Color blending
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// After a fragment shader has returned a color, it needs to be combined with the color that is already in the framebuffer. 
		// This transformation is known as color blending and there are two ways to do it:
		//		Mix the old and new value to produce a final color
		//		Combine the old and new value using a bitwise operation
		// There are two types of structs to configure color blending. The first struct, VkPipelineColorBlendAttachmentState contains
		// the configuration per attached framebuffer and the second struct, VkPipelineColorBlendStateCreateInfo contains the global
		// color blending settings. In our case we only have one framebuffer:
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

		// This per-framebuffer struct allows you to configure the first way of color blending. The operations that will be performed
		// are best demonstrated using the following pseudocode:
		//
		//		if (blendEnable) {
		//			finalColor.rgb = (srcColorBlendFactor * newColor.rgb) < colorBlendOp > (dstColorBlendFactor * oldColor.rgb);
		//			finalColor.a = (srcAlphaBlendFactor * newColor.a) < alphaBlendOp > (dstAlphaBlendFactor * oldColor.a);
		//		} else {
		//			finalColor = newColor;
		//		}
		//
		//		finalColor = finalColor & colorWriteMask;
		// 
		// If blendEnable is set to VK_FALSE, then the new color from the fragment shader is passed through unmodified. Otherwise,
		// the two mixing operations are performed to compute a new color. The resulting color is AND'd with the colorWriteMask 
		// to determine which channels are actually passed through. The most common way to use color blending is to implement 
		// alpha blending, where we want the new color to be blended with the old color based on its opacity.The finalColor 
		// should then be computed as follows:
		//
		//		finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
		//		finalColor.a = newAlpha.a;
		//
		// This can be accomplished with the following parameters:
		//
		//		colorBlendAttachment.blendEnable = VK_TRUE;
		//		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		//		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		//		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		//		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		//		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		//		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		//
		// You can find all of the possible operations in the VkBlendFactor and VkBlendOp enumerations in the specification.
		//
		//
		// The second structure references the array of structures for all of the framebuffers and allows you to set blend 
		// constants that you can use as blend factors in the aforementioned calculations.
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional
		// If you want to use the second method of blending (bitwise combination), then you should set logicOpEnable to VK_TRUE. 
		// The bitwise operation can then be specified in the logicOp field. Note that this will automatically disable the first 
		// method, as if you had set blendEnable to VK_FALSE for every attached framebuffer! The colorWriteMask will also be used 
		// in this mode to determine which channels in the framebuffer will actually be affected. It is also possible to disable 
		// both modes, as we've done here, in which case the fragment colors will be written to the framebuffer unmodified.

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Dynamic state
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// A limited amount of the state that we've specified in the previous structs can actually be changed without recreating 
		// the pipeline. Examples are the size of the viewport, line width and blend constants. If you want to do that, then you'll
		// have to fill in a VkPipelineDynamicStateCreateInfo structure like this:
		//
		//		VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_LINE_WIDTH };
		//
		//		VkPipelineDynamicStateCreateInfo dynamicState{};
		//		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		//		dynamicState.dynamicStateCount = 2;
		//		dynamicState.pDynamicStates = dynamicStates;
		//
		// This will cause the configuration of these values to be ignored and you will be required to specify the data at drawing 
		// time. We'll get back to this in a future chapter. This struct can be substituted by a nullptr later on if you don't have 
		// any dynamic state.

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Pipeline layout
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// You can use uniform values in shaders, which are globals similar to dynamic state variables that can be changed at drawing
		// time to alter the behavior of your shaders without having to recreate them. They are commonly used to pass the transformation
		// matrix to the vertex shader, or to create texture samplers in the fragment shader.
		// These uniform values need to be specified during pipeline creation by creating a VkPipelineLayout object.
		// We need to specify the descriptor set layout during pipeline creation to tell Vulkan which descriptors the shaders will be using
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}
		


		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Create the pipeline info struct
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// We start by referencing the array of VkPipelineShaderStageCreateInfo structs.
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;

		// Then we reference all of the structures describing the fixed-function stage.
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr; // Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr; // Optional

		// After that comes the pipeline layout, which is a Vulkan handle rather than a struct pointer.
		pipelineInfo.layout = pipelineLayout;

		// And finally we have the reference to the render pass and the index of the sub pass where this graphics pipeline will be 
		// used. It is also possible to use other render passes with this pipeline instead of this specific instance, but they have 
		// to be compatible with renderPass. We won't be using that feature
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;

		// There are actually two more parameters. Vulkan allows you to create a new graphics pipeline by deriving from an existing
		// pipeline. The idea of pipeline derivatives is that it is less expensive to set up pipelines when they have much 
		// functionality in common with an existing pipeline and switching between pipelines from the same parent can also be done
		// quicker. You can either specify the handle of an existing pipeline with basePipelineHandle or reference another pipeline
		// that is about to be created by index with basePipelineIndex. Right now there is only a single pipeline, so we'll simply 
		// specify a null handle and an invalid index. These values are only used if the VK_PIPELINE_CREATE_DERIVATIVE_BIT flag is
		// also specified in the flags field of VkGraphicsPipelineCreateInfo.
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		// And finally create the graphics pipeline:
		// The vkCreateGraphicsPipelines function actually has more parameters than the usual object creation functions in Vulkan.
		// It is designed to take multiple VkGraphicsPipelineCreateInfo objects and create multiple VkPipeline objects in a single 
		// call. The second parameter, for which we've passed the VK_NULL_HANDLE argument, references an optional VkPipelineCache 
		// object. A pipeline cache can be used to store and reuse data relevant to pipeline creation across multiple calls to 
		// vkCreateGraphicsPipelines and even across program executions if the cache is stored to a file. This makes it possible 
		// to significantly speed up pipeline creation at a later time.
		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}



		vkDestroyShaderModule(device, vertShaderModule, nullptr);
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// create the framebuffers corresponding to each retrieved image view from the swap chain
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void createFrameBuffers() {
		swapChainFramebuffers.resize(swapChainImageViews.size()); // resize the container to hold all of the framebuffers

		for (size_t i = 0; i < swapChainImageViews.size(); i++) { // iterate through the image views and create framebuffers from them
			VkImageView attachments[] = { swapChainImageViews[i] };

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass; // specify with which renderPass the framebuffer needs to be compatible. roughly means that they use the same number and type of attachments
			framebufferInfo.attachmentCount = 1; // attachmentCount and pAttachments parameters specify the VkImageView objects that should be bound to the respective attachment descriptions in the render pass pAttachment array.
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1; // refers to the number of layers in image arrays. Our swap chain images are single images, so the number of layers is 1.

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
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

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
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = static_cast<uint32_t>(swapChainImages.size());

		// We will allocate one of these descriptors for every frame. This pool size structure is referenced by the main VkDescriptorPoolCreateInfo
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;

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
		allocInfo.descriptorSetCount = swapChainImages.size();
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

			// If you're overwriting the whole buffer, like we are in this case, then it is is also possible to use the VK_WHOLE_SIZE 
			// value for the range. The configuration of descriptors is updated using the vkUpdateDescriptorSets function, which takes an
			// array of VkWriteDescriptorSet structs as parameter.
			//
			// The first two fields specify the descriptor set to update and the binding. We gave our uniform buffer binding index 0. 
			// Remember that descriptors can be arrays, so we also need to specify the first index in the array that we want to 
			// update. We're not using an array, so the index is simply 0.
			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = descriptorSets[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;

			// We need to specify the type of descriptor again. It's possible to update multiple descriptors at once in an array, 
			// starting at index dstArrayElement. The descriptorCount field specifies how many array elements you want to update.
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;

			// The last field references an array with descriptorCount structs that actually configure the descriptors. It depends 
			// on the type of descriptor which one of the three you actually need to use. The pBufferInfo field is used for 
			// descriptors that refer to buffer data, pImageInfo is used for descriptors that refer to image data, and 
			// pTexelBufferView is used for descriptors that refer to buffer views. Our descriptor is based on buffers, so we're 
			// using pBufferInfo.
			descriptorWrite.pBufferInfo = &bufferInfo;
			descriptorWrite.pImageInfo = nullptr; // Optional
			descriptorWrite.pTexelBufferView = nullptr; // Optional

			// The updates are applied using vkUpdateDescriptorSets. It accepts two kinds of arrays as parameters: an array of 
			// VkWriteDescriptorSet and an array of VkCopyDescriptorSet. The latter can be used to copy descriptors to each other, 
			// as its name implies.
			vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
		}
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// copy the contents of one buffer to another buffer
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
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

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = size;

		// Contents of buffers are transferred using the vkCmdCopyBuffer command. It takes the source and destination buffers as 
		// arguments, and an array of regions to copy. The regions are defined in VkBufferCopy structs and consist of a source 
		// buffer offset, destination buffer offset and size. It is not possible to specify VK_WHOLE_SIZE here, unlike the 
		// vkMapMemory command.
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		// This command buffer only contains the copy command, so we can stop recording right after that.
		vkEndCommandBuffer(commandBuffer);

		// Now execute the command buffer to complete the transfer
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		// Unlike the draw commands, there are no events we need to wait on this time. We just want to execute the transfer on the 
		// buffers immediately. There are again two possible ways to wait on this transfer to complete. We could use a fence and wait
		// with vkWaitForFences, or simply wait for the transfer queue to become idle with vkQueueWaitIdle. A fence would allow you 
		// to schedule multiple transfers simultaneously and wait for all of them complete, instead of executing one at a time. That
		// may give the driver more opportunities to optimize.
		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		// Don't forget to clean up the command buffer used for the transfer operation.
		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
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

		// Command buffers are allocated with the vkAllocateCommandBuffers function, which takes a VkCommandBufferAllocateInfo struct
		// as parameter that specifies the command pool and number of buffers to allocate. The level parameter specifies if the 
		// allocated command buffers are primary or secondary command buffers.
		//
		// VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for execution, but cannot be called from other command buffers.
		// VK_COMMAND_BUFFER_LEVEL_SECONDARY : Cannot be submitted directly, but can be called from primary command buffers.
		//
		// We won't make use of the secondary command buffer functionality here, but you can imagine that it's helpful to reuse 
		// common operations from primary command buffers.
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

		if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}

		// We begin recording a command buffer by calling vkBeginCommandBuffer with a small VkCommandBufferBeginInfo structure as 
		// an argument that specifies some details about the usage of this specific command buffer. 
		// The flags parameter specifies how we're going to use the command buffer. The following values are available:
		//
		//		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: The command buffer will be rerecorded right after executing it once.
		//		VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : This is a secondary command buffer that will be entirely within a single render pass.
		//		VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT : The command buffer can be resubmitted while it is also already pending execution.
		//
		// None of these flags are applicable for us right now.
		// The pInheritanceInfo parameter is only relevant for secondary command buffers. It specifies which state to inherit from 
		// the calling primary command buffers. If the command buffer was already recorded once, then a call to vkBeginCommandBuffer
		// will implicitly reset it.It's not possible to append commands to a buffer at a later time.
		for (size_t i = 0; i < commandBuffers.size(); i++) {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0; // Optional
			beginInfo.pInheritanceInfo = nullptr; // Optional

			if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
				throw std::runtime_error("failed to begin recording command buffer!");
			}

			// Drawing starts by beginning the render pass with vkCmdBeginRenderPass. The render pass is configured using some 
			// parameters in a VkRenderPassBeginInfo struct. The first parameters are the render pass itself and the attachments
			// to bind. We created a framebuffer for each swap chain image that specifies it as color attachment.
			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = swapChainFramebuffers[i];

			// The next two parameters define the size of the render area. The render area defines where shader loads and stores
			// will take place. The pixels outside this region will have undefined values. It should match the size of the 
			// attachments for best performance.
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = swapChainExtent;

			// The last two parameters define the clear values to use for VK_ATTACHMENT_LOAD_OP_CLEAR, which we used as load 
			// operation for the color attachment. Defined the clear color to simply be black with 100% opacity.
			VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			// The render pass can now begin. All of the functions that record commands can be recognized by their vkCmd prefix.
			// They all return void, so there will be no error handling until we've finished recording. The first parameter for 
			// every command is always the command buffer to record the command to. The second parameter specifies the details
			// of the render pass we've just provided. The final parameter controls how the drawing commands within the render 
			// pass will be provided. It can have one of two values:
			//
			//		VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed.
			//		VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : The render pass commands will be executed from secondary command buffers.
			// 
			// We will not be using secondary command buffers, so we'll go with the first option.
			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			// We can now bind the graphics pipeline. The second parameter specifies if the pipeline object is a graphics or compute pipeline. 
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			// The vkCmdBindVertexBuffers function is used to bind vertex buffers to bindings. The first two parameters, 
			// besides the command buffer, specify the offset and number of bindings we're going to specify vertex buffers for. 
			// The last two parameters specify the array of vertex buffers to bind and the byte offsets to start reading vertex data from. 
			VkBuffer vertexBuffers[] = { vertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);


			// An index buffer is bound with vkCmdBindIndexBuffer which has the index buffer, a byte offset into it, and the type of index 
			// data as parameters. the possible types are VK_INDEX_TYPE_UINT16 and VK_INDEX_TYPE_UINT32.
			vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);


			// the following block is deprecated but left in for notes.
			/*
			// We've now told Vulkan which operations to execute in the graphics pipeline and which attachment to use in the 
			// fragment shader, so all that remains is telling it to draw the triangle.
			// The actual vkCmdDraw is so simple because of all the information we specified in advance. It has the following 
			// parameters, aside from the command buffer:
			//
			//		vertexCount: however many vertices you want to draw. in this case, vertices.size() because that contains all of our vertices
			//		instanceCount : Used for instanced rendering, use 1 if you're not doing that.
			//		firstVertex : Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
			//		firstInstance : Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
			vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);
			*/

			// bind the right descriptor set for each swap chain image to the descriptors in the shader with cmdBindDescriptorSets. 
			// This needs to be done before the vkCmdDrawIndexed call
			//
			// Unlike vertex and index buffers, descriptor sets are not unique to graphics pipelines. Therefore we need to specify if we want
			// to bind descriptor sets to the graphics or compute pipeline. The next parameter is the layout that the descriptors are based on.
			// The next three parameters specify the index of the first descriptor set, the number of sets to bind, and the array of sets to 
			// bind. We'll get back to this in a moment. The last two parameters specify an array of offsets that are used for dynamic descriptors.
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

			// A call to this function is very similar to vkCmdDraw. The first two parameters specify the number of indices and the number of
			// instances. We're not using instancing, so just specify 1 instance. The number of indices represents the number of vertices 
			// that will be passed to the vertex buffer. The next parameter specifies an offset into the index buffer, using a value of 1
			// would cause the graphics card to start reading at the second index. The second to last parameter specifies an offset to add 
			// to the indices in the index buffer. The final parameter specifies an offset for instancing, which we're not using.
			vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

			// The render pass can now be ended.
			vkCmdEndRenderPass(commandBuffers[i]);

			// And we've finished recording the command buffer.
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
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(device); // dont touch resources that might still be in use

		cleanupSwapChain(); // clean up the old swap chain to make way for a newer, shinier one

		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
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
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}
};


int main() {
	CloneCraftApplication app;

	try {
		app.run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		PAUSE;
		return EXIT_FAILURE;
	}

	PAUSE;
	return EXIT_SUCCESS;
}