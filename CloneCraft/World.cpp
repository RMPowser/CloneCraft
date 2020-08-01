#pragma once
#include "World.h"
#include "Chunk.h"
#ifndef NOISE_STATIC
#define NOISE_STATIC
#endif // !NOISE_STATIC
#include "noise/noise.h"
#include "noiseutils.h"

//#define FRUSTUM_CULLING_ENABLED // currently broken 


std::unordered_map<Vec2XZ, Chunk> chunkMap;
bool forceVertexUpdate = false;


World::World(BlockDatabase* _blockdb, uint8_t _renderDistance, uint8_t _maxChunksPerFrame, int _seed) :
	terrainGenerator(CHUNK_WIDTH),
	seed(_seed){
	blockdb = _blockdb;
	renderDistance = _renderDistance;
	maxChunksPerFrame = _maxChunksPerFrame;
}

void World::update(Camera& cam, VkPhysicalDevice& physicalDevice, VkDevice& device, VkCommandPool& commandPool, VkQueue& graphicsQueue, std::vector<Vertex>& vertices, VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory, std::vector<uint32_t>& indices, VkBuffer& indexBuffer, VkDeviceMemory& indexBufferMemory) {
	camPositionNew = cam.position;
	camChunkCoordsNew = { (int)camPositionNew.x / CHUNK_WIDTH, (int)camPositionNew.z / CHUNK_WIDTH };
	updateLoadList();
	
	// if there is a change in the camera or the frustum
	if (camPositionOld != camPositionNew || camFrustum != cam.getFrustum()) {
		// get the newest data
		camFrustum = cam.getFrustum();
		camPositionOld = camPositionNew;
	}

	// if the camera has crossed into a new chunk or a vertex update is being forced
	if (camChunkCoordsOld != camChunkCoordsNew || forceVertexUpdate) {
		camChunkCoordsOld = camChunkCoordsNew;
		updateVisibleList();
		updateRenderList();
		updateUnloadList();

		updateVertexAndIndexBuffer(physicalDevice, device, commandPool, graphicsQueue, vertices, vertexBuffer, vertexBufferMemory, indices, indexBuffer, indexBufferMemory);
		forceVertexUpdate = false;
	}
	
}

void World::updateLoadList() {
	int numOfChunksLoaded = 0;

	// set bounds of how far out to render based on what chunk the player is in
	Vec2XZ lowChunkXZ = { camChunkCoordsNew.x - renderDistance, camChunkCoordsNew.z - renderDistance };
	Vec2XZ highChunkXZ = { camChunkCoordsNew.x + renderDistance, camChunkCoordsNew.z + renderDistance };

	// for each chunk around the player within render distance
	for (int x = lowChunkXZ.x; x <= highChunkXZ.x; x++) {
		for (int z = lowChunkXZ.z; z <= highChunkXZ.z; z++) {
			Vec2XZ chunkXZ{ x, z };

			// if the chunk is not already loaded
			if (!getChunk(x, z).isLoaded) {
				// if the chunk is not already in the load list
				if (!ChunkAlreadyExistsIn(chunkLoadList, chunkXZ)) {
					// put the chunk into the load list
					chunkLoadList.push_back(chunkXZ);
				}
			}
		}
	}

	// for each chunk in the load list
	for (int i = 0; i < chunkLoadList.size(); i++) {
		// if we havent hit the chunk load limit per frame
		if (numOfChunksLoaded != maxChunksPerFrame) {
			Vec2XZ chunk{ chunkLoadList[i].x, chunkLoadList[i].z };

			// load the chunk
			loadChunk(chunk);
			forceVertexUpdate = true;

			// Increase the chunks loaded count
			numOfChunksLoaded++;

			// add the chunk to the visible list because it is potentially visible
			visibleChunksList.push_back(chunk);

			// remove the chunk from the load list since it is now loaded
			chunkLoadList.erase(chunkLoadList.begin() + i);

			// subtract 1 from the index since the container size changed
			i--;
		}
		// if we have hit the chunk load limit per frame
		else {
			//stop looping
			break;
		}
	}
}

void World::updateVisibleList() {
	// for each chunk in the potentially visible list
	for (int i = 0; i < visibleChunksList.size(); i++) {
		Vec2XZ chunkXZ{ visibleChunksList[i].x, visibleChunksList[i].z };

		// get the chunk at that location
		Chunk* chunk = &getChunk(chunkXZ);

		// if the chunk is in the view frustum (if frustum culling is enabled)
		if (
			#ifdef FRUSTUM_CULLING_ENABLED 
				camFrustum.isBoxInFrustum(chunk->bbox)
			#else
				true
			#endif
			) {
			// if the chunk is loaded
			if (chunk->isLoaded) {
				// if the chunk is not already in the renderable list
				if (!ChunkAlreadyExistsIn(renderableChunksList, chunkXZ)) {
					// add the chunk to the renderable chunk list because it is able to be seen by the player
					renderableChunksList.push_back(chunkXZ);

					// remove it from the visible list
					visibleChunksList.erase(visibleChunksList.begin() + i);

					// subtract 1 from the index since the container size changed
					i--;
				}
				// if the chunk is already in the renderable list
				else {
					// remove it from the visible list
					visibleChunksList.erase(visibleChunksList.begin() + i);

					// subtract 1 from the index since the container size changed
					i--;
				}
			}
			// if the chunk is in the frustum but not yet loaded
			else {
				// do nothing. ie: wait for the chunk to be loaded in the next few frames.
			}
		}
		// if the chunk is not in the frustum at all
		else {
			// remove the chunk from the visible list because it is not visible to the player and should not be rendered
			visibleChunksList.erase(visibleChunksList.begin() + i);

			// subtract 1 from the index since the container size changed
			i--;
		}
	}

	//printf("visibleListSize: %zd \n", visibleChunksList.size());
}

void World::updateRenderList() {
	// for each chunk in the render list
	for (int i = 0; i < renderableChunksList.size(); i++) {
		Vec2XZ chunk{ renderableChunksList[i].x, renderableChunksList[i].z };

		// if the distance to the chunk is greater than renderDistance on either axis
		auto xDistance = abs(camChunkCoordsNew.x - chunk.x);
		auto zDistance = abs(camChunkCoordsNew.z - chunk.z);
		if (xDistance > renderDistance || zDistance > renderDistance) {
			// add the chunk to the unload list because its out of render distance
			chunkUnloadList.push_back(chunk);

			// remove the chunk from the renderable chunks list so it is no longer rendered
			renderableChunksList.erase(renderableChunksList.begin() + i);

			// subtract 1 from the index since the container size changed
			i--;
		}
	}
}

void World::updateUnloadList() {
	// for each chunk in the unload list
	for (int i = 0; i < chunkUnloadList.size(); i++) {
		Vec2XZ chunk{ chunkUnloadList[i].x, chunkUnloadList[i].z };

		// if the chunk is currently loaded and the distance to the chunk is greater than the renderDistance on either axis
		if (getChunk(chunk).isLoaded) {
			// unload the chunk
			unLoadChunk(chunk);

			// remove the chunk from the unload list since it is now unloaded
			chunkUnloadList.erase(chunkUnloadList.begin() + i);

			// subtract 1 from the index since the container size changed
			i--;
		}
	}
	
	//printf("unloadListSize: %zd\n", chunkUnloadList.size());
}





Vec2XZ World::getBlockXZ(int x, int z) {
	return { x % CHUNK_WIDTH, z % CHUNK_WIDTH };
}

Vec2XZ World::getChunkXZ(int x, int z) {
	return { x / CHUNK_WIDTH, z / CHUNK_WIDTH };
}

BlockId World::getBlock(int x, int y, int z) {
	auto blockPosition = getBlockXZ(x, z);
	auto chunkPosition = getChunkXZ(x, z);

	return getChunk(chunkPosition.x, chunkPosition.z).getBlock(blockPosition.x, y, blockPosition.z);
}

bool World::setBlock(BlockId id, int x, int y, int z) {
	auto blockPosition = getBlockXZ(x, z);
	auto chunkPosition = getChunkXZ(x, z);

	if (getChunk(chunkPosition.x, chunkPosition.z).setBlock(id, blockPosition.x, y, blockPosition.z)) {
		return true;
	}

	return false;
}

Chunk& World::getChunk(int x, int z) {
	Vec2XZ key{ x, z };
	if (!chunkExistsAt(x, z)) {
		Chunk chunk(*this, Vec2XZ{ x, z });
		chunkMap.emplace(key, std::move(chunk));
	}

	return chunkMap[key];
}

Chunk& World::getChunk(Vec2XZ chunkPos) {
	if (!chunkExistsAt(chunkPos.x, chunkPos.z)) {
		Chunk chunk(*this, Vec2XZ{ chunkPos.x, chunkPos.z });
		chunkMap.emplace(chunkPos, std::move(chunk));
	}

	return chunkMap[chunkPos];
}

void World::loadChunk(Vec2XZ chunkPos) {
	Chunk* chunk = &getChunk(chunkPos.x, chunkPos.z);
	
	// generate the terrain image
	auto image = terrainGenerator.GetTerrain(chunkPos.x, chunkPos.z, seed);
	
	// sample the image at x and z coords to get y coord
	for (int x = 0; x < CHUNK_WIDTH; x++) {
		for (int z = 0; z < CHUNK_WIDTH; z++) {
			int y = image->GetValue(x, z).red;
			chunk->setBlock(BlockId::Grass, x, y, z);
		}
	}

	// generate spheres of dirt....
	//for (int z = 0; z < CHUNK_WIDTH; z++) {
	//	for (int y = 0; y < CHUNK_WIDTH; y++) {
	//		for (int x = 0; x < CHUNK_WIDTH; x++) {
	//			if (sqrt((float)(x - CHUNK_WIDTH / 2) * (x - CHUNK_WIDTH / 2) + (y - CHUNK_WIDTH / 2) * (y - CHUNK_WIDTH / 2) + (z - CHUNK_WIDTH / 2) * (z - CHUNK_WIDTH / 2)) <= CHUNK_WIDTH / 2) {
	//				chunk->setBlock(BlockId::Grass, x, y, z);
	//			}
	//		}
	//	}
	//}

	chunk->generateVerticesAndIndices();

	chunk->isLoaded = true;
}

void World::updateChunk(Vec2XZ chunkPos) {
	Chunk* chunk = &getChunk(chunkPos);

	chunk->generateVerticesAndIndices();
	forceVertexUpdate = true;
}

void World::unLoadChunk(Vec2XZ chunkPos) {
	// todo: Save chunk to file eventually
	if (chunkExistsAt(chunkPos.x, chunkPos.z)) {
		Vec2XZ key{ chunkPos.x, chunkPos.z };
		chunkMap.erase(key);
	}
}

bool World::chunkExistsAt(int x, int z) {
	Vec2XZ key{ x, z };
	return chunkMap.find(key) != chunkMap.end();
}

double World::distanceToChunk(Vec2XZ chunkXZ) {
	return sqrt(((camChunkCoordsNew.x - chunkXZ.x) * (camChunkCoordsNew.x - chunkXZ.x)) + ((camChunkCoordsNew.z - chunkXZ.z) * (camChunkCoordsNew.z - chunkXZ.z)));
}

bool World::ChunkAlreadyExistsIn(std::vector<Vec2XZ> v, Vec2XZ elem) {
	// for each chunk in the list
	for (int i = 0; i < v.size(); i++) {
		// break if the chunk is in the load list
		if (v[i] == elem) {
			return true;
			break;
		}
	}

	return false;
}









uint32_t findMemoryType(VkPhysicalDevice& physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	// First we need to query info about the available types of memory using vkGetPhysicalDeviceMemoryProperties.
	// The VkPhysicalDeviceMemoryProperties structure has two arrays memoryTypes and memoryHeaps. Memory heaps are distinct
	// memory resources like dedicated VRAM and swap space in RAM for when VRAM runs out. The different types of memory exist 
	// within these heaps. Right now we'll only concern ourselves with the type of memory and not the heap it comes from, but
	// you can imagine that this can affect performance.
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	// find a memory type that is suitable for the buffer itself.
	// The typeFilter parameter will be used to specify the bit field of memory types that are suitable. That means that we can
	// find the index of a suitable memory type by simply iterating over them and checking if the corresponding bit is set to 1.
	// but we also need to be able to write our vertex data to that memory. The memoryTypes array consists of VkMemoryType
	// structs that specify the heap and properties of each type of memory. The properties define special features of the
	// memory, like being able to map it so we can write to it from the CPU. This property is indicated with
	// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, but we also need to use the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT property. We'll see
	// why when we map the memory.
	// We may have more than one desirable property, so we should check if the result of the bitwise AND is not just non-zero, 
	// but equal to the desired properties bit field. If there is a memory type suitable for the buffer that also has all of 
	// the properties we need, then we return its index, otherwise we throw an exception.
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

void createBuffer(VkPhysicalDevice& physicalDevice, VkDevice& device, VkDeviceSize& size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
	// Creating a buffer requires us to fill a VkBufferCreateInfo structure. because ofc it does...
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

	// size specifies the size of the buffer in bytes.
	bufferInfo.size = size;

	// usage indicates the purposes the data in the buffer will be used for. It is possible to specify multiple purposes using a bitwise OR.
	bufferInfo.usage = usage;

	// Just like the images in the swap chain, buffers can also be owned by a specific queue family or be shared between multiple at the same time. For now, the buffer will only be used from the graphics queue, so we can stick to exclusive access.
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	// create the buffer
	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		__debugbreak();
		throw std::runtime_error("failed to create buffer!");
	}

	// query buffer memory requirements
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	// use those requirements to findMemoryType for allocation info
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

	// allocate the vertex buffer memory
	if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		__debugbreak();
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	// bind the memory to the buffer. The first three parameters are self-explanatory and the fourth parameter is the offset 
	// within the region of memory. Since this memory is allocated specifically for this the vertex buffer, the offset is simply
	// 0. If the offset is non-zero, then it is required to be divisible by memRequirements.alignment.
	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

VkCommandBuffer beginSingleTimeCommands(VkDevice& device, VkCommandPool& commandPool) {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void endSingleTimeCommands(VkDevice& device, VkQueue& graphicsQueue, VkCommandPool& commandPool, VkCommandBuffer& commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void copyBuffer(VkDevice& device, VkCommandPool& commandPool, VkQueue& graphicsQueue, VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize& size) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer);
}





void World::updateVertexAndIndexBuffer(VkPhysicalDevice& physicalDevice, VkDevice& device, VkCommandPool& commandPool, VkQueue& graphicsQueue, std::vector<Vertex>& vertices, VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory, std::vector<uint32_t>& indices, VkBuffer& indexBuffer, VkDeviceMemory& indexBufferMemory) {
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
	vkDestroyBuffer(device, indexBuffer, nullptr);
	vkFreeMemory(device, indexBufferMemory, nullptr);

	
	vertices.clear();
	indices.clear();

	// for each chunk in the render list
	for (int i = 0; i < renderableChunksList.size(); i++) {
		Vec2XZ chunkXZ{ renderableChunksList[i].x, renderableChunksList[i].z };
		// get the chunk
		Chunk* chunk = &getChunk(chunkXZ);

		/*if (!chunk->isVisible) {
			break;
		}*/

		// get the chunks data
		auto verts = chunk->vertices;
		auto inds = chunk->indices;

		// save the offset for the indices
		auto offset = vertices.size();

		vertices.insert(vertices.end(), verts.begin(), verts.end());

		// account for the offset into the vertices vector and store the indices for later
		for (int i = 0; i < inds.size(); i++) {
			auto ind(inds[i] + offset);
			indices.push_back(ind);
		}
	}



	// time to start creating the actual buffer	
	VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer vertexStagingBuffer;
	VkDeviceMemory vertexStagingBufferMemory;

	createBuffer(physicalDevice, device, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexStagingBuffer, vertexStagingBufferMemory);

	void* vertexData;
	vkMapMemory(device, vertexStagingBufferMemory, 0, vertexBufferSize, 0, &vertexData);
	memcpy(vertexData, vertices.data(), (size_t)vertexBufferSize);
	vkUnmapMemory(device, vertexStagingBufferMemory);

	createBuffer(physicalDevice, device, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

	// use copyBuffer() to move the vertex data to the device local buffer
	copyBuffer(device, commandPool, graphicsQueue, vertexStagingBuffer, vertexBuffer, vertexBufferSize);

	// After copying the data from the staging buffer to the device buffer, we should clean up the staging buffer since it is no longer needed.
	vkDestroyBuffer(device, vertexStagingBuffer, nullptr);
	vkFreeMemory(device, vertexStagingBufferMemory, nullptr);

	// and do the same for the index buffer
	VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer indexStagingBuffer;
	VkDeviceMemory indexStagingBufferMemory;
	createBuffer(physicalDevice, device, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, indexStagingBuffer, indexStagingBufferMemory);

	void* indexData;
	vkMapMemory(device, indexStagingBufferMemory, 0, indexBufferSize, 0, &indexData);
	memcpy(indexData, indices.data(), (size_t)indexBufferSize);
	vkUnmapMemory(device, indexStagingBufferMemory);

	createBuffer(physicalDevice, device, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

	copyBuffer(device, commandPool, graphicsQueue, indexStagingBuffer, indexBuffer, indexBufferSize);

	vkDestroyBuffer(device, indexStagingBuffer, nullptr);
	vkFreeMemory(device, indexStagingBufferMemory, nullptr);
}

