#pragma once
#include "Layer.h"
#include "AABB.h"

class Chunk {
public:
	Chunk();
	Chunk(World& _world, Vec2XZ pos);
	~Chunk() {};

	BlockId getBlock(int x, int y, int z);
	bool setBlock(BlockId id, int x, int y, int z);
	bool isBlockOutOfBounds(int x, int y, int z);
	void generateVerticesAndIndices();

	std::array<Layer, CHUNK_HEIGHT> layers;
	Vec2XZ position;
	const World* world;
	bool isLoaded = false;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	AABB bbox;
private:
	
};