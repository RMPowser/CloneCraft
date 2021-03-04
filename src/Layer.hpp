#ifndef LAYER_HPP
#define LAYER_HPP

#include "Block.hpp"

class Layer {
public:
	BlockType blocks[AppGlobals::CHUNK_WIDTH][AppGlobals::CHUNK_WIDTH];

	Layer()
	{
		for (size_t i = 0; i < AppGlobals::CHUNK_WIDTH; i++)
		{
			for (size_t j = 0; j < AppGlobals::CHUNK_WIDTH; j++)
			{
				blocks[i][j] = BlockType::Air;
			}
		}
	}

	~Layer() {}

	BlockType GetBlock(Vec4 blockPos)
	{
		return blocks[(int)blockPos.x][(int)blockPos.z];
	}

	bool SetBlock(BlockType id, Vec4 blockPos)
	{
		blocks[(int)blockPos.x][(int)blockPos.z] = id;
		return true;
	}
};
#endif // LAYER_HPP