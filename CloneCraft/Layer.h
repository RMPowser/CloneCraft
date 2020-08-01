#pragma once
#include "Block.h"
#include "World.h"

class Layer {
public:
	Layer();
	~Layer() {};

	BlockId getBlock(int x, int z);
	bool setBlock(BlockId id, int x, int z);

	std::array<std::array<BlockId, CHUNK_WIDTH>, CHUNK_WIDTH> blocks;
private:
};