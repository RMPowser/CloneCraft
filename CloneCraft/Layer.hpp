#ifndef LAYER_HPP
#define LAYER_HPP

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

	BlockId GetBlock(Vec4 blockPos)
	{
		return blocks[(int)blockPos.x][(int)blockPos.z];
	}

	bool SetBlock(BlockId id, Vec4 blockPos)
	{
		blocks[(int)blockPos.x][(int)blockPos.z] = id;
		return true;
	}
};
#endif // LAYER_HPP