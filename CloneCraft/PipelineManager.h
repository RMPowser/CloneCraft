#pragma once
#include "vulkan/vulkan.h"
#include "DeviceManager.h"
#include "ShaderManager.h"
#include "DescriptorManager.h"
#include "RenderManager.h"
#include "Vertex.h"

class SwapchainManager;

class PipelineManager {
public:
	PipelineManager(DeviceManager& deviceManager, SwapchainManager& swapchainManager, ShaderManager& shaderManager, DescriptorManager& descriptorManager, RenderManager& renderManager);
	~PipelineManager();
	void createGraphicsPipeline();

	VkPipeline&			GetGraphicsPipeline()	{ return graphicsPipeline; }
	VkPipelineLayout&	GetPipelineLayout()		{ return pipelineLayout; }

private:
	VkPipeline graphicsPipeline;
	VkPipelineLayout pipelineLayout;

	DeviceManager* deviceManagerPointer;
	SwapchainManager* swapchainManagerPointer;
	ShaderManager* shaderManagerPointer;
	DescriptorManager* descriptorManagerPointer;
	RenderManager* renderManagerPointer;

};