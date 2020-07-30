#pragma once
#include <string>
#include <vector>

struct AppConfig {
	AppConfig(uint32_t _windowX, uint32_t _windowY, uint32_t _FOV, std::string _windowTitle, uint32_t _maxFramesInFlight, std::vector<const char*> _deviceExtensions) {
		windowX = _windowX;
		windowY = _windowY;
		FOV = _FOV;
		windowTitle = _windowTitle;
		maxFramesInFlight = _maxFramesInFlight;
		deviceExtensions = _deviceExtensions;
	}
	~AppConfig() {}
	uint32_t windowX;
	uint32_t windowY;
	uint32_t FOV;
	std::string windowTitle;
	uint32_t maxFramesInFlight; // defines how many frames should be processed concurrently. must be non-zero and positive
	std::vector<const char*> deviceExtensions;
	std::vector<const char*> validationLayers;
	int seed = -1;
};