#pragma once
#include "Camera.h"
#include "Player.h"
#include "World.h"
#include "Chunk.h"
#include "Block.h"
#include "UBO.h"
#include "glm.h"
#include "Ray.h"
#include "Controller.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <optional>
#include <set>
#include <cstdint> // Necessary for UINT32_MAX
#include <algorithm>
#include <glm/glm.hpp>
#include <array>

#include "shaderc/shaderc.hpp" // needed for compiling shaders at runtime

// glsl Vertex shader
const char* vertexShaderCode = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
)";

// glsl fragment shader
const char* fragmentShaderCode = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler, fragTexCoord);
}
)";

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// define globals
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Player player;
Controller controller;
bool cameraMat4Updated = false;
bool playerMat4Updated = false;
bool firstMouse = true;


class Renderer {
	GW::SYSTEM::GWindow				window;
	GW::GRAPHICS::GVulkanSurface	vulkan;
	GW::CORE::GEventReceiver		shutdownTrigger;

	VkDevice						device;
	VkPhysicalDevice				physicalDevice;
	std::vector<Vertex>				vertices;
	VkBuffer						vertexBuffer = NULL;
	VkDeviceMemory					vertexBufferMemory = NULL;
	std::vector<uint32_t>			indices;
	VkBuffer						indexBuffer = NULL;
	VkDeviceMemory					indexBufferMemory = NULL;
	VkShaderModule					vertShaderModule;
	VkShaderModule					fragShaderModule;
	VkPipeline						graphicsPipeline;
	VkPipelineLayout				pipelineLayout;
	VkDescriptorPool				descriptorPool;
	std::vector<VkDescriptorSet>	descriptorSets;
	VkDescriptorSetLayout			descriptorSetLayout;
	std::vector<VkBuffer>			uniformBuffers;
	std::vector<VkDeviceMemory>		uniformBuffersMemory;
	std::vector<VkImage>			textureImages;
	std::vector<VkDeviceMemory>		textureImagesMemory;
	std::vector<VkImageView>		textureImageViews;
	VkSampler						textureSampler;
	VkQueue							graphicsQueue;
	VkCommandPool					commandPool;
	size_t							currentFrame = 0; // keeps track of the current frame. no way! :D

	AppConfig						config;
	Camera							camera;
	BlockDatabase					blockdb;
	World							world;



	//VkImage							depthImage;
	//VkDeviceMemory					depthImageMemory;
	//VkImageView						depthImageView;
public:
	Renderer(GW::SYSTEM::GWindow& _window, GW::GRAPHICS::GVulkanSurface _vulkan, AppConfig _config) :
	camera(_config),
	world(blockdb, config) {
		config = _config;
		camera.hookEntity(player);
		player.world = &world;
		window = _window;
		vulkan = _vulkan;
		unsigned int width, height;
		window.GetClientWidth(width);
		window.GetClientHeight(height);
		unsigned int swapchainImageCount;
		vulkan.GetSwapchainImageCount(swapchainImageCount);
		vulkan.GetDevice((void**)&device);
		vulkan.GetPhysicalDevice((void**)&physicalDevice);
		vulkan.GetCommandPool((void**)&commandPool);
		vulkan.GetGraphicsQueue((void**)&graphicsQueue);

		/***************** SHADER INTIALIZATION ******************/
		// Intialize runtime shader compiler GLSL -> SPIRV
		shaderc::Compiler shaderCompiler;
		shaderc::CompileOptions compilerOptions;
#ifndef NDEBUG
		compilerOptions.SetGenerateDebugInfo();
#endif
		// create vertex shader
		shaderc::SpvCompilationResult result = shaderCompiler.CompileGlslToSpv(vertexShaderCode, shaderc_vertex_shader, "main.vert", compilerOptions);
		if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
			std::cout << "Vertex Shader Errors: " << result.GetErrorMessage() << std::endl;
		}

		// load vertex shader into vulkan
		GvkHelper::create_shader_module(device, result.GetLength(), (char*)result.begin(), &vertShaderModule);

		// create fragment shader
		result = shaderCompiler.CompileGlslToSpv(fragmentShaderCode, shaderc_fragment_shader, "main.vert", compilerOptions);
		if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
			std::cout << "Fragment Shader Errors: " << result.GetErrorMessage() << std::endl;
		}
		
		// load fragment shader into vulkan
		GvkHelper::create_shader_module(device, result.GetLength(), (char*)result.begin(), &fragShaderModule);

		/***************** PIPELINE INTIALIZATION ******************/
		// create pipeline and layout
		VkRenderPass renderPass;
		vulkan.GetRenderPass((void**)&renderPass);
		VkPipelineShaderStageCreateInfo shaderStageCreateInfo[2] = {};
		// vertex shader
		shaderStageCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStageCreateInfo[0].module = vertShaderModule;
		shaderStageCreateInfo[0].pName = "main";
		// fragment shader
		shaderStageCreateInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStageCreateInfo[1].module = fragShaderModule;
		shaderStageCreateInfo[1].pName = "main";

		// assembly state
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
		inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

		// Vertex Input State
		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();
		VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
		vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
		vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		// viewport state
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)width;
		viewport.height = (float)height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = { width, height };
		VkPipelineViewportStateCreateInfo viewportCreateInfo{};
		viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportCreateInfo.viewportCount = 1;
		viewportCreateInfo.pViewports = &viewport;
		viewportCreateInfo.scissorCount = 1;
		viewportCreateInfo.pScissors = &scissor;

		// rasterization state
		VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo{};
		rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationCreateInfo.lineWidth = 1.0f;
		rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationCreateInfo.depthClampEnable = VK_FALSE;
		rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
		rasterizationCreateInfo.depthBiasClamp = 0.0f;
		rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
		rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;

		// multisampling state
		VkPipelineMultisampleStateCreateInfo multisampleCreateInfo{};
		multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
		multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleCreateInfo.minSampleShading = 1.0f;
		multisampleCreateInfo.pSampleMask = VK_NULL_HANDLE;
		multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

		// depth stencil state
		VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo{};
		depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilCreateInfo.depthTestEnable = VK_TRUE;
		depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
		depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
		depthStencilCreateInfo.minDepthBounds = 0.0f;
		depthStencilCreateInfo.maxDepthBounds = 1.0f;
		depthStencilCreateInfo.stencilTestEnable = VK_FALSE;

		// color blending attachment and state
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_MAX;
		VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo{};
		colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendCreateInfo.logicOpEnable = VK_FALSE;
		colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
		colorBlendCreateInfo.attachmentCount = 1;
		colorBlendCreateInfo.pAttachments = &colorBlendAttachment;
		colorBlendCreateInfo.blendConstants[0] = 0.0f;
		colorBlendCreateInfo.blendConstants[1] = 0.0f;
		colorBlendCreateInfo.blendConstants[2] = 0.0f;
		colorBlendCreateInfo.blendConstants[3] = 0.0f;

		// dynamic state
		VkDynamicState dynamicState[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR }; // By setting these we do not need to re-create the pipeline on Resize
		VkPipelineDynamicStateCreateInfo dynamicCreateInfo = {};
		dynamicCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicCreateInfo.dynamicStateCount = 2;
		dynamicCreateInfo.pDynamicStates = dynamicState;

		// descriptor set layout
		VkDescriptorSetLayoutBinding layoutBindings[2]{};
		// ubo layout binding
		layoutBindings[0].binding = 0;
		layoutBindings[0].descriptorCount = 1;
		layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBindings[0].pImmutableSamplers = nullptr;
		layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		// sampler layout binding
		layoutBindings[1].binding = 1;
		layoutBindings[1].descriptorCount = 1;
		layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBindings[1].pImmutableSamplers = nullptr;
		layoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 2;
		layoutInfo.pBindings = layoutBindings;
		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}

		// descriptor pipeline layout
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
		if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		// pipeline state. all of the previous is just setup for this
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStageCreateInfo;
		pipelineInfo.pVertexInputState = &vertexInputCreateInfo;
		pipelineInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
		pipelineInfo.pViewportState = &viewportCreateInfo;
		pipelineInfo.pRasterizationState = &rasterizationCreateInfo;
		pipelineInfo.pMultisampleState = &multisampleCreateInfo;
		pipelineInfo.pDepthStencilState = &depthStencilCreateInfo;
		pipelineInfo.pColorBlendState = &colorBlendCreateInfo;
		pipelineInfo.pDynamicState = &dynamicCreateInfo;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;
		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}

		// create texture images
		for (int i = 1; i < (int)BlockId::NUM_TYPES; i++) {
			int index = i - 1; // this is the index for the textureImages[] array, not the BlockId's
			auto imageSize = blockdb.blockDataFor((BlockId)i).getTexture().size;
			auto pixels = blockdb.blockDataFor((BlockId)i).getTexture().image;
			VkExtent3D texExtent;
			texExtent.width = blockdb.blockDataFor((BlockId)i).getTexture().width;
			texExtent.height = blockdb.blockDataFor((BlockId)i).getTexture().height;
			texExtent.depth = 1;

			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;

			// The buffer should be in host visible memory so that we can map it and it should be usable as a transfer source so that we can copy it to an image later on
			GvkHelper::create_buffer(physicalDevice, device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

			// We can then directly copy the pixel values that we got from the image loading library to the buffer
			GvkHelper::write_to_buffer(device, stagingBufferMemory, pixels, imageSize);

			VkImage image;
			VkDeviceMemory imageMemory;
			GvkHelper::create_image(physicalDevice, device, texExtent, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, nullptr, &image, &imageMemory);

			textureImages.push_back(image);
			textureImagesMemory.push_back(imageMemory);

			GvkHelper::transition_image_layout(device, commandPool, graphicsQueue, 1, textureImages[index], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			GvkHelper::copy_buffer_to_image(device, commandPool, graphicsQueue, stagingBuffer, textureImages[index], texExtent);

			// To be able to start sampling from the texture image in the shader, we need one last transition to prepare it for shader access
			GvkHelper::transition_image_layout(device, commandPool, graphicsQueue, 1, textureImages[index], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			vkDestroyBuffer(device, stagingBuffer, nullptr);
			vkFreeMemory(device, stagingBufferMemory, nullptr);

			VkImageView imageView;
			GvkHelper::create_image_view(device, textureImages[index], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1, nullptr, &imageView);

			textureImageViews.push_back(imageView);
		}

		// create texture sampler
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_NEAREST;
		samplerInfo.minFilter = VK_FILTER_NEAREST;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 16.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;
		if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}

		// create uniform buffers
		// A uniform buffer is a buffer that is made accessible in a read-only fashion to shaders so that the shaders can read constant 
		// parameter data. This is another example of a step that you have to perform in a Vulkan program that you wouldn't have to do 
		// in another graphics API.
		CreateUniformBuffers(swapchainImageCount);
		

		// create descriptor pool to allocate descriptor sets from
		// We first need to describe which descriptor types our descriptor sets are going to contain and how many of them, using VkDescriptorPoolSize structures
		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(swapchainImageCount);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(swapchainImageCount);
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(swapchainImageCount);
		// The structure has an optional flag similar to command pools that determines if individual descriptor sets can be freed or
		// not: VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT. We're not going to touch the descriptor set after creating it, 
		// so we don't need this flag. leave flags to its default value of 0.
		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}

		// create descriptor sets, one for each swap chain image, all with the same layout
		std::vector<VkDescriptorSetLayout> layouts(swapchainImageCount, descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = swapchainImageCount;
		allocInfo.pSetLayouts = layouts.data();
		descriptorSets.resize(swapchainImageCount);
		if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) { // You don't need to explicitly clean up descriptor sets, because they will be automatically freed when the descriptor pool is destroyed. The call to vkAllocateDescriptorSets will allocate descriptor sets, each with one uniform buffer descriptor.
			throw std::runtime_error("failed to allocate descriptor sets!");
		}
		// The descriptor sets have been allocated now, but the descriptors within still need to be configured. We'll now add a loop to populate every descriptor
		// Descriptors that refer to buffers, like our uniform buffer descriptor, are configured with a VkDescriptorBufferInfo struct. 
		// This structure specifies the buffer and the region within it that contains the data for the descriptor.
		for (size_t i = 0; i < swapchainImageCount; i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = textureImageViews[0];
			imageInfo.sampler = textureSampler;
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
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}



		/***************** CLEANUP / SHUTDOWN ******************/
		// GVulkanSurface will inform us when to release any allocated resources and when the swapchain has been recreated
		shutdownTrigger.Create(vulkan, [&]() {
			if (+shutdownTrigger.Find(GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES, true)) {
				CleanUp(); // unlike D3D we must be careful about destroy timing
			}
			if (+shutdownTrigger.Find(GW::GRAPHICS::GVulkanSurface::Events::REBUILD_PIPELINE, true)) {
				CleanUpUniformBuffers(uniformBuffers.size());

				unsigned int newSwapchainImageCount;
				vulkan.GetSwapchainImageCount(newSwapchainImageCount);
				CreateUniformBuffers(newSwapchainImageCount);
			}
		});
	}

	void Render(float deltaTime) {
		update(deltaTime);
		drawFrame(deltaTime);

		VkFence renderFence{};
		vulkan.GetRenderFence(-1, (void**)renderFence);
	}

private:
	void update(float dt) {
		auto acceleration = &(player.acceleration);
		auto rotation = &(player.rotation);
		float moveAcceleration = config.moveAcceleration;
		float jumpHeight = config.jumpHeight;
		int buildRange = config.buildRange;

		if (controller.forwardPressed) {
			acceleration->x += -cos(RADIAN * (rotation->y + 90.f)) * moveAcceleration;
			acceleration->z += -sin(RADIAN * (rotation->y + 90.f)) * moveAcceleration;
		}
		if (controller.backPressed) {
			acceleration->x += cos(RADIAN * (rotation->y + 90.f)) * moveAcceleration;
			acceleration->z += sin(RADIAN * (rotation->y + 90.f)) * moveAcceleration;
		}
		if (controller.leftPressed) {
			acceleration->x += -cos(RADIAN * (rotation->y)) * moveAcceleration;
			acceleration->z += -sin(RADIAN * (rotation->y)) * moveAcceleration;
		}
		if (controller.rightPressed) {
			acceleration->x += cos(RADIAN * (rotation->y)) * moveAcceleration;
			acceleration->z += sin(RADIAN * (rotation->y)) * moveAcceleration;
		}
		if (controller.upPressed) {
			if (player.isOnGround && !player.isFlying) {
				acceleration->y += jumpHeight;
			}
			else if (player.isFlying) {
				acceleration->y += moveAcceleration;
			}
		}
		if (controller.downPressed) {
			acceleration->y += -moveAcceleration;
		}
		if (controller.flyToggleNew != controller.flyToggleOld) {
			player.isFlying = controller.flyToggleNew;
			controller.flyToggleOld = controller.flyToggleNew;
		}
		if (controller.speedModifierPressed) {
			moveAcceleration = 1.5;
		} else {
			moveAcceleration = 1;
		}
		if (controller.leftClicked) {
			Vec3 camPosition{};
			camPosition.x = camera.position.x;
			camPosition.y = camera.position.y;
			camPosition.z = camera.position.z;

			for (Ray ray(camPosition, player.rotation); ray.getLength() <= buildRange; ray.step(0.05f)) {
				int x = static_cast<int>(ray.getEnd().x);
				int y = static_cast<int>(ray.getEnd().y);
				int z = static_cast<int>(ray.getEnd().z);

				auto block = world.getBlock(x, y, z);

				if (block != BlockId::Air) {
					if (world.setBlock(BlockId::Air, x, y, z)) {
						world.updateChunk(World::getChunkXZ(x, z));
						break;
					} else {
						__debugbreak();
						throw new std::runtime_error("unable to destroy block!");
					}
				}
			}

			controller.leftClicked = false;
		}
		if (controller.rightClicked) {
			Vec3 camPosition{};
			camPosition.x = camera.position.x;
			camPosition.y = camera.position.y;
			camPosition.z = camera.position.z;

			Vec3 lastRayPosition;

			for (Ray ray(camPosition, player.rotation); ray.getLength() <= buildRange; ray.step(0.05f)) {
				int x = static_cast<int>(ray.getEnd().x);
				int y = static_cast<int>(ray.getEnd().y);
				int z = static_cast<int>(ray.getEnd().z);

				auto block = world.getBlock(x, y, z);

				if (block != BlockId::Air) {
					if (world.setBlock(BlockId::Grass, lastRayPosition.x, lastRayPosition.y, lastRayPosition.z)) {
						world.updateChunk(World::getChunkXZ(lastRayPosition.x, lastRayPosition.z));
						break;
					} else {
						__debugbreak();
						throw new std::runtime_error("unable to destroy block!");
					}
				}
				lastRayPosition = ray.getEnd();
			}
			controller.rightClicked = false;
		}



		player.update(dt, controller);
		camera.update();


//#ifndef NDEBUG
//		SetStdOutCursorPosition(0, 0);
//		std::cout << "                                                               ";
//		std::cout << "\n                                                               ";
//		std::cout << "\n                                                               ";
//		std::cout << "\n                                                               ";
//		std::cout << "\n                                                               ";
//		std::cout << "\n                                                               ";
//		std::cout << "\n                                                               ";
//		SetStdOutCursorPosition(0, 0);
//		std::cout << "Player";
//		std::cout << "\nx: " << player.bbox.position.x << "\ty: " << player.bbox.position.y << "\tz: " << player.bbox.position.z;
//		std::cout << "\nspeed: " << config.moveAcceleration;
//		std::cout << "\nacceleration x: " << acceleration->x << " y: " << acceleration->y << " z: " << acceleration->z;
//		std::cout << "\nvelocity x: " << player.velocity.x << " y: " << player.velocity.y << " z: " << player.velocity.z;
//		std::cout << "\ndt: " << dt << "\n";
//#endif // DEBUG
		

		

		world.update(camera, vertices, indices);
	}

	void SetStdOutCursorPosition(short CoordX, short CoordY)
		//our function to set the cursor position.
	{
		HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		COORD position = { CoordX,CoordY };

		SetConsoleCursorPosition(hStdout, position);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Function will perform the following operations:
	//
	//		Acquire an image from the swap chain
	//		Execute the command buffer with that image as attachment in the framebuffer
	//		Return the image to the swap chain for presentation
	//
	// Each of these events is set in motion using a single function call, but they are executed asynchronously. The function calls 
	// will return before the operations are actually finished and the order of execution is also undefined. Unfortunate, because 
	// each of the operations depends on the previous one finishing. There are two ways of synchronizing swap chain events: fences 
	// and semaphores. They're both objects that can be used for coordinating operations by having one operation signal and another 
	// operation wait for a fence or semaphore to go from the unsignaled to signaled state. The difference is that the state of 
	// fences can be accessed from your program using calls like vkWaitForFences and semaphores cannot be. Fences are mainly designed
	// to synchronize your application itself with rendering operation, whereas semaphores are used to synchronize operations within
	// or across command queues. We want to synchronize the queue operations of draw commands and presentation, which makes 
	// semaphores the best fit.
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void drawFrame(float dt) {
		uint32_t currentImage;
		vulkan.GetSwapchainCurrentImage(currentImage);

		if (world.verticesAndIndicesUpdated) {
			world.verticesAndIndicesUpdated = false;

			vkDestroyBuffer(device, vertexBuffer, nullptr);
			vkFreeMemory(device, vertexBufferMemory, nullptr);
			vkDestroyBuffer(device, indexBuffer, nullptr);
			vkFreeMemory(device, indexBufferMemory, nullptr);

			// time to start creating the actual vertex and index buffers
			VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();

			VkBuffer vertexStagingBuffer;
			VkDeviceMemory vertexStagingBufferMemory;
			GvkHelper::create_buffer(physicalDevice, device, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexStagingBuffer, &vertexStagingBufferMemory);
			GvkHelper::write_to_buffer(device, vertexStagingBufferMemory, vertices.data(), (unsigned int)vertexBufferSize);

			GvkHelper::create_buffer(physicalDevice, device, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, &vertexBufferMemory);

			// use copyBuffer() to move the vertex data to the device local buffer
			GvkHelper::copy_buffer(device, commandPool, graphicsQueue, vertexStagingBuffer, vertexBuffer, vertexBufferSize);

			// After copying the data from the staging buffer to the device buffer, we should clean up the staging buffer since it is no longer needed.
			vkDestroyBuffer(device, vertexStagingBuffer, nullptr);
			vkFreeMemory(device, vertexStagingBufferMemory, nullptr);

			// and do the same for the index buffer
			VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();

			VkBuffer indexStagingBuffer;
			VkDeviceMemory indexStagingBufferMemory;
			GvkHelper::create_buffer(physicalDevice, device, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &indexStagingBuffer, &indexStagingBufferMemory);
			GvkHelper::write_to_buffer(device, indexStagingBufferMemory, vertices.data(), (unsigned int)indexBufferSize);

			GvkHelper::create_buffer(physicalDevice, device, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, &indexBufferMemory);

			GvkHelper::copy_buffer(device, commandPool, graphicsQueue, indexStagingBuffer, indexBuffer, indexBufferSize);

			vkDestroyBuffer(device, indexStagingBuffer, nullptr);
			vkFreeMemory(device, indexStagingBufferMemory, nullptr);
		}
		

		updateUniformBuffer(currentImage, dt);

		VkCommandBuffer commandBuffer;
		vulkan.GetCommandBuffer(currentImage, (void**)&commandBuffer);

		// what is the current client area dimensions?
		unsigned int width, height;
		window.GetClientWidth(width);
		window.GetClientHeight(height);

		// setup the pipeline's dynamic settings
		VkViewport viewport;
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = width;
		viewport.height = height;
		viewport.minDepth = 0;
		viewport.maxDepth = 1;
		
		VkRect2D scissor = { {0, 0}, {width, height} };
		scissor.offset = { 0, 0 };
		scissor.extent = { width, height };

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentImage], 0, nullptr);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// clean up any memory in reverse order of declaration
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void CleanUp() {
		// wait untill everything has completed
		vkDeviceWaitIdle(device);

		vkDestroySampler(device, textureSampler, nullptr);
		
		for (size_t i = 0; i < textureImages.size(); i++) {
			vkDestroyImageView(device, textureImageViews[i], nullptr);
			vkDestroyImage(device, textureImages[i], nullptr);
			vkFreeMemory(device, textureImagesMemory[i], nullptr);
		}

		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vkDestroyBuffer(device, indexBuffer, nullptr);
		vkFreeMemory(device, indexBufferMemory, nullptr);
		vkDestroyBuffer(device, vertexBuffer, nullptr);
		vkFreeMemory(device, vertexBufferMemory, nullptr);

		vkDestroyShaderModule(device, vertShaderModule, nullptr);
		vkDestroyShaderModule(device, fragShaderModule, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyPipeline(device, graphicsPipeline, nullptr);
	}

	void CreateUniformBuffers(unsigned int _swapchainImageCount) {
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);
		uniformBuffers.resize(_swapchainImageCount);
		uniformBuffersMemory.resize(_swapchainImageCount);
		for (size_t i = 0; i < _swapchainImageCount; i++) {
			GvkHelper::create_buffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniformBuffers[i], &uniformBuffersMemory[i]);
		}
	}

	void CleanUpUniformBuffers(unsigned int _swapchainImageCount) {
		for (size_t i = 0; i < _swapchainImageCount; i++) {
			vkDestroyBuffer(device, uniformBuffers[i], nullptr);
			vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// This function will generate a new transformation every frame based on the cameras position
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void updateUniformBuffer(uint32_t currentImage, float dt) {
		// define the model, view, and projection transformations in the uniform buffer object. 
		UniformBufferObject ubo{};
		ubo.model = glm::mat4(1.0f);
		ubo.view = camera.getViewMatrix();
		ubo.proj = camera.getProjMatrix();

		// GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted. The easiest way to 
		// compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix. If you don't do 
		// this, then the image will be rendered upside down.
		ubo.proj[1][1] *= -1;

		// All of the transformations are defined now, so we can copy the data in the uniform buffer object to the current uniform
		// buffer. This happens in exactly the same way as we did for vertex buffers, except without a staging buffer
		void* data;
		vkMapMemory(device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, uniformBuffersMemory[currentImage]);
	}
};