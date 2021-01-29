#pragma once
#include "Layer.hpp"

class Chunk
{
public:
	Layer layers[256];
	GW::MATH::GVECTORF position;
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	GW::MATH::GAABBCEF bbox;
	bool isLoaded = false;


	Chunk() {}
	Chunk(GW::MATH::GVECTORF pos) :
		position(pos),
		bbox({ static_cast<float>(AppGlobals::CHUNK_WIDTH), static_cast<float>(AppGlobals::CHUNK_HEIGHT), static_cast<float>(AppGlobals::CHUNK_WIDTH) })
	{
		GW::MATH::GVector::AddVectorF(bbox.extent, GW::MATH::GVECTORF{ pos.x * AppGlobals::CHUNK_WIDTH, 0, pos.z * AppGlobals::CHUNK_WIDTH, 0 }, bbox.extent); // bbox position is in world coordinates, not chunk coordinates
	}
	~Chunk() {}

	BlockId getBlock(int x, int y, int z)
	{
		if (isBlockOutOfBounds(x, y, z))
		{
			return BlockId::Air;
		}

		return layers[y].getBlock(x, z);
	}

	bool setBlock(BlockId id, int x, int y, int z)
	{
		if (!isBlockOutOfBounds(x, y, z))
		{
			if (layers[y].setBlock(id, x, z))
			{
				return true;
			}
		}

		return false;
	}

	bool isBlockOutOfBounds(int x, int y, int z)
	{
		if (x >= AppGlobals::CHUNK_WIDTH)
			return true;
		if (z >= AppGlobals::CHUNK_WIDTH)
			return true;

		if (x < 0)
			return true;
		if (y < 0)
			return true;
		if (z < 0)
			return true;

		if (y >= AppGlobals::CHUNK_HEIGHT)
		{
			return true;
		}

		return false;
	}
};