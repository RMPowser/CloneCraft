#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "UBO.hpp"
#include <chrono>
#include <fstream>
#include <vector>
#include <array>

#include "shaderc/shaderc.hpp" // needed for compiling shaders at runtime


/***************** DEFINE GLOBALS ******************/

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
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

void main() {
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
	fragTexCoord = inTexCoord;
}
)";

// glsl fragment shader
const char* fragmentShaderCode = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

vec4 GammaCorrection (vec4 color, float gamma) {
  return pow(color, vec4(gamma));
}

void main() {
	outColor = GammaCorrection(texture(texSampler, fragTexCoord), 1 / 2.2);
}
)";

class Renderer {
	// gonna need these gateware objects
	GW::CORE::GEventReceiver		vulkanEventResponder;
	GW::INPUT::GBufferedInput		bufferedInput;
	GW::CORE::GEventReceiver		inputEventResponder;

	// gonna need these vulkan objects
	VkDevice						device;
	VkPhysicalDevice				physicalDevice;
	std::vector<Vertex>				vertices;
	std::vector<uint32_t>			indices;
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

	// vulkan doesnt like it when you try to destroy an uninitialized buffer or device memory.
	// we initialize them to NULL so that the first frame draw doesnt crash.
	VkBuffer						vertexBuffer = NULL;
	VkDeviceMemory					vertexBufferMemory = NULL;
	VkBuffer						indexBuffer = NULL;
	VkDeviceMemory					indexBufferMemory = NULL;

	Camera							camera;



public:
	Renderer()
	{
		auto& window = AppGlobals::window;
		camera.HookEntity(AppGlobals::player);
		auto width = window.GetClientWidth();
		auto height = window.GetClientHeight();
		unsigned int swapchainImageCount;
		window.vulkan.GetSwapchainImageCount(swapchainImageCount);
		window.vulkan.GetDevice((void**)&device);
		window.vulkan.GetPhysicalDevice((void**)&physicalDevice);
		window.vulkan.GetCommandPool((void**)&commandPool);
		window.vulkan.GetGraphicsQueue((void**)&graphicsQueue);

		/***************** SHADER INTIALIZATION ******************/
		// Initialize runtime shader compiler GLSL -> SPIRV
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

		// vertex shader module
		GvkHelper::create_shader_module(device, result.GetLength(), (char*)result.begin(), &vertShaderModule);

		// create fragment shader
		result = shaderCompiler.CompileGlslToSpv(fragmentShaderCode, shaderc_fragment_shader, "main.vert", compilerOptions);
		if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
			std::cout << "Fragment Shader Errors: " << result.GetErrorMessage() << std::endl;
		}

		// fragment shader module
		GvkHelper::create_shader_module(device, result.GetLength(), (char*)result.begin(), &fragShaderModule);

		/***************** PIPELINE INTIALIZATION ******************/
		// create pipeline and layout
		VkRenderPass renderPass;
		window.vulkan.GetRenderPass((void**)&renderPass);
		VkPipelineShaderStageCreateInfo shaderStageCreateInfo[2] = {};
		// load vertex shader into vulkan
		shaderStageCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStageCreateInfo[0].module = vertShaderModule;
		shaderStageCreateInfo[0].pName = "main";
		// load fragment shader into vulkan
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
		for (int i = 1; i < (int)BlockType::NUM_TYPES; i++) {
			// top
			{
				int index = i - 1; // this is the index for the textureImages[] array, not the BlockType's
				auto imageSize = AppGlobals::world.blockdb.GetTextureAtlas().GetSize();
				auto pixels = AppGlobals::world.blockdb.GetTextureAtlas().GetPixels();
				VkExtent3D texExtent;
				texExtent.width = AppGlobals::world.blockdb.GetTextureAtlas().GetWidth();
				texExtent.height = AppGlobals::world.blockdb.GetTextureAtlas().GetHeight();
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
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
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
		CreateDescriptorPool(swapchainImageCount);

		// create descriptor sets, one for each swap chain image, all with the same layout
		CreateDescriptorSets(swapchainImageCount);


		/***************** CLEANUP / SHUTDOWN / REBUILD PIPELINE ******************/
		// GVulkanSurface will inform us when to release any allocated resources and when the swapchain has been recreated
		vulkanEventResponder.Create(window.vulkan, [&]() {
			if (+vulkanEventResponder.Find(GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES, true)) {
				CleanUp(); // be careful about destroy timing
			}
			if (+vulkanEventResponder.Find(GW::GRAPHICS::GVulkanSurface::Events::REBUILD_PIPELINE, true)) {
				vkDeviceWaitIdle(device);
				CleanUpUniformBuffers();
				CleanUpDescriptorPool();
				// descriptor sets are automatically destroyed when the descriptor pool is

				unsigned int newSwapchainImageCount;
				window.vulkan.GetSwapchainImageCount(newSwapchainImageCount);
				CreateUniformBuffers(newSwapchainImageCount);
				CreateDescriptorPool(newSwapchainImageCount);
				CreateDescriptorSets(newSwapchainImageCount);
				camera.RecreateProjectionMatrix();
			}
			});
	}

	void Render(float deltaTime) {
		unsigned int currentImage;
		AppGlobals::window.vulkan.GetSwapchainCurrentImage(currentImage);

		if (AppGlobals::world.verticesAndIndicesUpdated) {
			AppGlobals::world.verticesAndIndicesUpdated = false;

			vkDeviceWaitIdle(device);

			vkDestroyBuffer(device, vertexBuffer, nullptr);
			vkFreeMemory(device, vertexBufferMemory, nullptr);
			vkDestroyBuffer(device, indexBuffer, nullptr);
			vkFreeMemory(device, indexBufferMemory, nullptr);

			// time to start creating the actual vertex buffer	
			VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();

			VkBuffer vertexStagingBuffer;
			VkDeviceMemory vertexStagingBufferMemory;

			GvkHelper::create_buffer(physicalDevice, device, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexStagingBuffer, &vertexStagingBufferMemory);
			GvkHelper::write_to_buffer(device, vertexStagingBufferMemory, vertices.data(), (unsigned int)vertexBufferSize);
			GvkHelper::create_buffer(physicalDevice, device, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, &vertexBufferMemory);
			GvkHelper::copy_buffer(device, commandPool, graphicsQueue, vertexStagingBuffer, vertexBuffer, vertexBufferSize);

			vkDestroyBuffer(device, vertexStagingBuffer, nullptr);
			vkFreeMemory(device, vertexStagingBufferMemory, nullptr);

			// and the index buffer
			VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();

			VkBuffer indexStagingBuffer;
			VkDeviceMemory indexStagingBufferMemory;

			GvkHelper::create_buffer(physicalDevice, device, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &indexStagingBuffer, &indexStagingBufferMemory);
			GvkHelper::write_to_buffer(device, indexStagingBufferMemory, indices.data(), (unsigned int)indexBufferSize);
			GvkHelper::create_buffer(physicalDevice, device, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, &indexBufferMemory);
			GvkHelper::copy_buffer(device, commandPool, graphicsQueue, indexStagingBuffer, indexBuffer, indexBufferSize);

			vkDestroyBuffer(device, indexStagingBuffer, nullptr);
			vkFreeMemory(device, indexStagingBufferMemory, nullptr);
		}
		else // always load the chunk the player is in so we never have a 0 size vertex buffer
		{
			if (vertices.size() == 0 || indices.size() == 0)
			{
				auto playerChunkXZ = AppGlobals::world.getChunkXZ(AppGlobals::player.position);
				auto c = AppGlobals::world.getChunk(playerChunkXZ);

				vertices.clear();
				indices.clear();

				if (!c->isLoaded)
				{
					AppGlobals::world.initChunk(playerChunkXZ);
				}

				// get the chunks data
				auto verts = c->vertices;
				auto inds = c->indices;

				// save the offset for the indices
				auto offset = vertices.size();

				vertices.insert(vertices.end(), verts.begin(), verts.end());

				// account for the offset into the vertices vector and store the indices for later
				for (int i = 0; i < inds.size(); i++)
				{
					auto ind(inds[i] + offset);
					indices.push_back(ind);
				}

				vkDeviceWaitIdle(device);

				vkDestroyBuffer(device, vertexBuffer, nullptr);
				vkFreeMemory(device, vertexBufferMemory, nullptr);
				vkDestroyBuffer(device, indexBuffer, nullptr);
				vkFreeMemory(device, indexBufferMemory, nullptr);

				// time to start creating the actual vertex buffer	
				VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();

				VkBuffer vertexStagingBuffer;
				VkDeviceMemory vertexStagingBufferMemory;

				GvkHelper::create_buffer(physicalDevice, device, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexStagingBuffer, &vertexStagingBufferMemory);
				GvkHelper::write_to_buffer(device, vertexStagingBufferMemory, vertices.data(), (unsigned int)vertexBufferSize);
				GvkHelper::create_buffer(physicalDevice, device, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, &vertexBufferMemory);
				GvkHelper::copy_buffer(device, commandPool, graphicsQueue, vertexStagingBuffer, vertexBuffer, vertexBufferSize);

				vkDestroyBuffer(device, vertexStagingBuffer, nullptr);
				vkFreeMemory(device, vertexStagingBufferMemory, nullptr);

				// and the index buffer
				VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();

				VkBuffer indexStagingBuffer;
				VkDeviceMemory indexStagingBufferMemory;

				GvkHelper::create_buffer(physicalDevice, device, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &indexStagingBuffer, &indexStagingBufferMemory);
				GvkHelper::write_to_buffer(device, indexStagingBufferMemory, indices.data(), (unsigned int)indexBufferSize);
				GvkHelper::create_buffer(physicalDevice, device, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, &indexBufferMemory);
				GvkHelper::copy_buffer(device, commandPool, graphicsQueue, indexStagingBuffer, indexBuffer, indexBufferSize);

				vkDestroyBuffer(device, indexStagingBuffer, nullptr);
				vkFreeMemory(device, indexStagingBufferMemory, nullptr);
			}
		}

		updateUniformBuffer(currentImage, deltaTime);

		VkCommandBuffer commandBuffer;
		AppGlobals::window.vulkan.GetCommandBuffer(-1, (void**)&commandBuffer);

		// what is the current client area dimensions?
		auto width = AppGlobals::window.GetClientWidth();
		auto height = AppGlobals::window.GetClientHeight();

		// setup the pipeline's dynamic settings
		VkViewport viewport;
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = width;
		viewport.height = height;
		viewport.minDepth = 0;
		viewport.maxDepth = 1;

		VkRect2D scissor;
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
		vkCmdDrawIndexed(commandBuffer, (uint32_t)(indices.size()), 1, 0, 0, 0);
	}

	void update(float deltaTime) {
		auto& controller = AppGlobals::controller;
		auto& player = AppGlobals::player;
		auto& world = AppGlobals::world;

		controller.update();
		player.update(deltaTime, camera);
		camera.Update();
		

		PrintGameInfo(true, deltaTime);
		

		world.update(camera, vertices, indices);
	}

private:
	void PrintGameInfo(bool b, float dt) {
		if (b) {
			float fps = 1.f / dt;
			auto& controller = AppGlobals::controller;
			auto& player = AppGlobals::player;
			auto playerPosition = AppGlobals::player.position;
			auto playerBoxPosition = AppGlobals::player.bbox.position;
			auto cameraPosition = camera.position;
			SetStdOutCursorPosition(0, 0);
			printf(
R"(Player Info										
position:		x: %4f		y: %4f		z: %4f		
camera:			x: %4f		y: %4f		z: %4f		
bbox			x: %4f		y: %4f		z: %4f		
rotation:		x: %4f		y: %4f		z: %4f		
rotationCam:	x: %4f		y: %4f		z: %4f	
up pressed: %d										
dt:		%f                                              
fps:	%f                                         

			)", playerPosition.x, playerPosition.y, playerPosition.z,
			cameraPosition.x, cameraPosition.y, cameraPosition.z,
			playerBoxPosition.x, playerBoxPosition.y, playerBoxPosition.z,
			player.rotation.x, player.rotation.y, player.rotation.z,
			camera.rotation.x, camera.rotation.y, camera.rotation.z,
			controller.keys[G_KEY_SPACE],
			dt,
			fps);
		}
	}


	void SetStdOutCursorPosition(short CoordX, short CoordY) {
		HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		COORD position = { CoordX,CoordY };

		SetConsoleCursorPosition(hStdout, position);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// clean up any memory in reverse order of declaration
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void CleanUp() {
		// wait until everything has completed
		vkDeviceWaitIdle(device);

		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

		CleanUpUniformBuffers();

		vkDestroyDescriptorPool(device, descriptorPool, nullptr);

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
	}

	void CreateUniformBuffers(unsigned int _swapchainImageCount) {
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);
		uniformBuffers.resize(_swapchainImageCount);
		uniformBuffersMemory.resize(_swapchainImageCount);
		for (size_t i = 0; i < _swapchainImageCount; i++) {
			GvkHelper::create_buffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &(uniformBuffers[i]), &(uniformBuffersMemory[i]));
		}
	}

	void CleanUpUniformBuffers() {
		for (size_t i = 0; i < uniformBuffers.size(); i++) {
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
		ubo.model = Mat4::IdentityMatrix();
		ubo.view = camera.getViewMatrix();
		ubo.proj = camera.getProjMatrix();


		// All of the transformations are defined now, so we can copy the data in the uniform buffer object to the current uniform
		// buffer. This happens in exactly the same way as we did for vertex buffers, except without a staging buffer
		GvkHelper::write_to_buffer(device, uniformBuffersMemory[currentImage], &ubo, sizeof(ubo));
	}

	void CreateDescriptorPool(unsigned int swapchainImageCount) {
		// We first need to describe which descriptor types our descriptor sets are going to contain and how many of them, using VkDescriptorPoolSize structures
		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = swapchainImageCount;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = swapchainImageCount;
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = swapchainImageCount;
		// The structure has an optional flag similar to command pools that determines if individual descriptor sets can be freed or
		// not: VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT. We're not going to touch the descriptor set after creating it, 
		// so we don't need this flag. leave flags to its default value of 0.
		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	void CleanUpDescriptorPool() {
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	}

	void CreateDescriptorSets(unsigned int swapchainImageCount) {
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
	}
};

#endif // RENDERER_HPP