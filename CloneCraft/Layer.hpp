#pragma once
#include "Block.hpp"

class Layer {
public:
	BlockId blocks[16][16];

	Layer()
	{
		for (size_t i = 0; i < AppGlobals::CHUNK_WIDTH; i++)
		{
			for (size_t j = 0; j < AppGlobals::CHUNK_WIDTH; j++)
			{
				blocks[i][j] = BlockId::Air;
			}
		}
	}

	~Layer() {}

	BlockId getBlock(int x, int z)
	{
		return blocks[x][z];
	}

	bool setBlock(BlockId id, int x, int z)
	{
		blocks[x][z] = id;
		return true;
	}
};