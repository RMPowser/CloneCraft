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

	BlockId getBlock(GW::MATH::GVECTORF blockPos)
	{
		return blocks[static_cast<int>(blockPos.x)][static_cast<int>(blockPos.z)];
	}

	bool setBlock(BlockId id, GW::MATH::GVECTORF blockPos)
	{
		blocks[static_cast<int>(blockPos.x)][static_cast<int>(blockPos.z)] = id;
		return true;
	}
};