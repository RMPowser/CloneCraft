#pragma once
#include "Block.h"
#include "Camera.h"
#include "TerrainGenerator.h"
#include <vector>
#include <unordered_map>

class Chunk;

const int CHUNK_WIDTH = 16;
const int CHUNK_HEIGHT = 256;

struct Vec2XZ {
	int x;
	int z;

	Vec2XZ() {
		x = 0;
		z = 0;
	}

	Vec2XZ(int _x, int _z) {
		x = _x;
		z = _z;
	}

	bool operator==(const Vec2XZ& other) const {
		return x == other.x && z == other.z;
	}

	bool operator!=(const Vec2XZ& other) const {
		return !(x == other.x && z == other.z);
	}
};

namespace std {
	template<> struct hash<Vec2XZ> {
		size_t operator()(Vec2XZ const& vec) const {
			return ((std::hash<int>{}(vec.x) ^ (std::hash<int>{}(vec.z))));
		}
	};
}

class World {
public:
	World(BlockDatabase* _blockdb, uint8_t _renderDistance, uint8_t _maxChunksPerFrame, int _seed);
	~World() {}


	void update(Camera& cam, VkPhysicalDevice& physicalDevice, VkDevice& device, VkCommandPool& commandPool, VkQueue& graphicsQueue, std::vector<Vertex>& vertices, VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory, std::vector<uint32_t>& indices, VkBuffer& indexBuffer, VkDeviceMemory& indexBufferMemory);
	void updateLoadList();
	void updateUnloadList();
	void updateVisibleList();
	void updateRenderList();
	void updateVertexAndIndexBuffer(VkPhysicalDevice& physicalDevice, VkDevice& device, VkCommandPool& commandPool, VkQueue& graphicsQueue, std::vector<Vertex>& vertices, VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory, std::vector<uint32_t>& indices, VkBuffer& indexBuffer, VkDeviceMemory& indexBufferMemory);
	static Vec2XZ getBlockXZ(int x, int z);
	static Vec2XZ getChunkXZ(int x, int z);
	BlockId getBlock(int x, int y, int z);
	bool setBlock(BlockId id, int x, int y, int z);
	Chunk& getChunk(int x, int z);
	Chunk& getChunk(Vec2XZ chunkPos);
	void loadChunk(int x, int z);
	void loadChunk(Vec2XZ chunkPos);
	void unLoadChunk(int x, int z);
	void unLoadChunk(Vec2XZ chunkPos);
	bool chunkExistsAt(int x, int z);
	bool ChunkAlreadyExistsIn(std::vector<Vec2XZ> v, Vec2XZ elem);
	double distanceToChunk(Vec2XZ chunkXZ);
	
	BlockDatabase* blockdb;
	std::vector<Vec2XZ> visibleChunksList;
	std::vector<Vec2XZ> renderableChunksList;
	std::vector<Vec2XZ> chunkLoadList;
	std::vector<Vec2XZ> chunkUnloadList;
	glm::vec3 camPositionOld;
	glm::vec3 camPositionNew;
	Vec2XZ camChunkCoordsOld;
	Vec2XZ camChunkCoordsNew;
	// !!! CHUNKMAP DECLARED IN CPP !!! //
private:
	ViewFrustum camFrustum;
	bool frustumCullingEnabled = true;
	uint8_t renderDistance;
	uint8_t maxChunksPerFrame;
	TerrainGenerator terrainGenerator;
	int seed;
};



