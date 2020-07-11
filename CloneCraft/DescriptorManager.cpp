#include "DescriptorManager.h"
#include "TextureManager.h"

DescriptorManager::DescriptorManager(DeviceManager& deviceManager, TextureManager& textureManager, BufferManager& bufferManager) {
	deviceManagerPointer = &deviceManager;
	textureManagerPointer = &textureManager;
	bufferManagerPointer = &bufferManager;
}

DescriptorManager::~DescriptorManager() {
	vkDestroyDescriptorSetLayout(deviceManagerPointer->GetLogicalDevice(), descriptorSetLayout, nullptr);
}

void DescriptorManager::createDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(deviceManagerPointer->GetLogicalDevice(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void DescriptorManager::createDescriptorPool() {
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(textureManagerPointer->GetSwapchainImages().size());
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(textureManagerPointer->GetSwapchainImages().size());

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(textureManagerPointer->GetSwapchainImages().size());
	poolInfo.maxSets = static_cast<uint32_t>(textureManagerPointer->GetSwapchainImages().size());;

	// The descriptor pool should be destroyed when the swap chain is recreated because it depends on the number of images. see cleanupSwapChain()
	if (vkCreateDescriptorPool(deviceManagerPointer->GetLogicalDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void DescriptorManager::createDescriptorSets() {
	std::vector<VkDescriptorSetLayout> layouts(textureManagerPointer->GetSwapchainImages().size(), descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = (uint32_t)textureManagerPointer->GetSwapchainImages().size();
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(textureManagerPointer->GetSwapchainImages().size());
	if (vkAllocateDescriptorSets(deviceManagerPointer->GetLogicalDevice(), &allocInfo, descriptorSets.data()) != VK_SUCCESS) { // You don't need to explicitly clean up descriptor sets, because they will be automatically freed when the descriptor pool is destroyed. The call to vkAllocateDescriptorSets will allocate descriptor sets, each with one uniform buffer descriptor.
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < textureManagerPointer->GetSwapchainImages().size(); i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = bufferManagerPointer->GetUniformBuffers()[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(GlobalHelpers::UniformBufferObject);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = textureManagerPointer->GetTextureImageView();
		imageInfo.sampler = textureManagerPointer->GetTextureSampler();

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(deviceManagerPointer->GetLogicalDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}