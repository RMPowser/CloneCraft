#pragma once
#include "vulkan/vulkan.h"
#include "DeviceManager.h"
#include <iostream>
#include <vector>

class ShaderManager {
public:
	ShaderManager(DeviceManager& deviceManager);
	~ShaderManager();
	VkShaderModule createShaderModule(const std::vector<char>& code);

private:
	DeviceManager* deviceManagerPointer;

};