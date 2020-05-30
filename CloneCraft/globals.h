#pragma once
#include "INCLUDER.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// define globals
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;
const char* WINDOW_TITLE = "CloneCraft   :^)";
const int MAX_FRAMES_IN_FLIGHT = 2; // defines how many frames should be processed concurrently
const std::string MODEL_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "textures/viking_room.png";

size_t currentFrame = 0; // keeps track of the current frame. no way! :D

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

GLFWwindow* window;
VkInstance instance;
VkSurfaceKHR surface;
VkDebugUtilsMessengerEXT debugMessenger;
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkDevice device;
std::vector<Vertex> vertices;
std::vector<uint32_t> indices;