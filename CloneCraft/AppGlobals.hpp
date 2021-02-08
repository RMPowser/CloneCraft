#pragma once
#include <string>
#include <vector>

namespace AppGlobals {
	static unsigned int windowX = 800;
	static unsigned int windowY = 600;
	static float FOV = 75.0f;
	static unsigned int maxFramesInFlight = 2; // defines how many frames should be processed concurrently. must be non-zero and positive
	static const std::string windowTitle = "CloneCraft  :^)";
	static std::vector<const char*> deviceExtensions = { "VK_KHR_swapchain" };
	static std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };
	static int seed = -1;
	static unsigned short renderDistance = 3; // render distance in chunks
	static unsigned short asyncNumChunksPerFrame = 2; // max number of chunks to load per frame
	static float moveAcceleration = 1.0f;
	static float buildRange = 5.0f;
	static float jumpHeight = 12.0f; // this just happens to feel like a good number at 60fps
	static float mouseSensitivity = 0.1f;
	static float mouseBound = 88.0f;
	static const int CHUNK_WIDTH = 16;
	static const int CHUNK_HEIGHT = 256;

	constexpr float PI = 3.14159265358979f;
	constexpr float RADIAN = PI / 180;
}

// include world down here so that it has access to AppGlobals
#include "World.hpp"

namespace AppGlobals 
{
	static World world;
}