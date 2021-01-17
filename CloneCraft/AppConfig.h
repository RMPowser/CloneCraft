#pragma once
#include <string>
#include <vector>

struct AppConfig {
	uint32_t windowX = 800;
	uint32_t windowY = 600;
	uint32_t FOV = 75;
	std::string windowTitle = "CloneCraft";
	uint32_t maxFramesInFlight = 2; // defines how many frames should be processed concurrently. must be non-zero and positive
	std::vector<const char*> deviceExtensions = { "VK_KHR_swapchain" };
	std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };
	int seed = -1;
	uint8_t renderDistance = 2; // render distance in chunks
	uint8_t asyncNumChunksPerFrame = 2; // max number of chunks to load per frame
	float moveAcceleration; // set this in the update function
	float buildRange = 5;
	float jumpHeight = 12; // this just happens to feel like a good number at 60fps
	float mouseSensitivity = 0.1f; 
};