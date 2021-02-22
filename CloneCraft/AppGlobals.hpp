#pragma once
#define GATEWARE_ENABLE_CORE
#define GATEWARE_ENABLE_SYSTEM
#define GATEWARE_ENABLE_GRAPHICS
#define GATEWARE_ENABLE_AUDIO
#define GATEWARE_ENABLE_MATH
#define GATEWARE_ENABLE_MATH2D
#define GATEWARE_ENABLE_INPUT
#include "../Gateware/Gateware.h"
#include <string>
#include <vector>
//#include "C:\Users\Ryan\source\repos\gateware-development\SingleHeader\Gateware.h"




void PrintDebugInfo(const char* format, ...) {
#ifndef NDEBUG
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
#endif
}


namespace AppGlobals {
	static const std::string windowTitle = "CloneCraft  :^)";
	static float FOV = 75.0f; // vertical fov
	static unsigned int maxFramesInFlight = 2; // defines how many frames should be processed concurrently. must be non-zero and positive
	static std::vector<const char*> deviceExtensions = { "VK_KHR_swapchain" };
	static std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };
	static int seed = -1;
	static unsigned short renderDistance = 3; // render distance in chunks
	static unsigned short asyncNumChunksPerFrame = 2; // max number of chunks to load per frame
	static float playerSpeed = 5.0f;
	static float gravity = -9.81f * playerSpeed;
	static float buildRange = 5.0f;
	static float jumpHeight = 1.5f; // 1.5 blocks high
	static float mouseSensitivity = 0.05f;
	static float mouseBound = 89.0f;
	static const int CHUNK_WIDTH = 16;
	static const int CHUNK_HEIGHT = 256;
}


// include window down here so that it has access to AppGlobals
#include "Window.hpp" 
namespace AppGlobals {
	static Window window;
}


#include "Math.hpp" // matrix needs to know about the window

// same for world
#include "World.hpp"
namespace AppGlobals {
	static World world;
}

// same for controller
#include "Controller.hpp"
namespace AppGlobals {
	static Controller controller;
}

// same for player
#include "Player.hpp"
namespace AppGlobals {
	static Player player;
}
