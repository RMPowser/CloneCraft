#pragma once
#include "vulkan/vulkan.h"
#include "DeviceManager.h"
#include <vector>
#include <array>

class TextureManager;
class BufferManager;

class DescriptorManager {
public:
	DescriptorManager(DeviceManager& deviceManager, TextureManager& textureManager, BufferManager& bufferManager);
	~DescriptorManager();
	void createDescriptorSetLayout();
	void createDescriptorPool();
	void createDescriptorSets();

	VkDescriptorSetLayout&			GetDescriptorSetLayout()	{ return descriptorSetLayout; }
	VkDescriptorPool&				GetDescriptorPool()			{ return descriptorPool; }
	std::vector<VkDescriptorSet>&	GetDescriptorSets()			{ return descriptorSets; }

private:
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	DeviceManager* deviceManagerPointer;
	TextureManager* textureManagerPointer;
	BufferManager* bufferManagerPointer;

};

