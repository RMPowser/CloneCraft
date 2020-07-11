#pragma once
#include "GLFW/glfw3.h"
#include "GlobalHelpers.h"
#include <iostream>
#include <vector>

class InstanceManager {
public:
	InstanceManager(std::vector<const char*> _validationLayers);
	~InstanceManager();
	void CreateVKInstance();
	VkInstance& GetInstance();
	bool isValidationLayersEnabled();

private:
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif
	VkInstance instance;

	std::vector<const char*> validationLayers;

	bool checkValidationLayerSupport();
	std::vector<const char*> getRequiredExtensions();
};