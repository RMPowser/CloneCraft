#pragma once
#include "Layer.hpp"

class Chunk {
public:
	Layer layers[256];
	GW::MATH::GVECTORF position;
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	GW::MATH::GAABBMMF bbox;
	bool isLoaded = false;


	Chunk() {}
	Chunk(GW::MATH::GVECTORF pos) {
		position = { floor(pos.x), 0, floor(pos.z), 0 }; // chunks never have a y position
		bbox.min = position;
		bbox.max = { AppGlobals::CHUNK_WIDTH / 2, AppGlobals::CHUNK_HEIGHT, AppGlobals::CHUNK_WIDTH / 2, 0 };
	}
	~Chunk() {}

	BlockId getBlock(GW::MATH::GVECTORF blockPos) {
		if (isBlockOutOfBounds(blockPos)) {
			return BlockId::Air;
		}

		return layers[static_cast<int>(blockPos.y)].getBlock(blockPos);
	}

	bool setBlock(BlockId id, GW::MATH::GVECTORF blockPos) {
		if (!isBlockOutOfBounds(blockPos)) {
			if (layers[static_cast<int>(blockPos.y)].setBlock(id, blockPos)) {
				return true;
			}
		}

		return false;
	}

	bool isBlockOutOfBounds(GW::MATH::GVECTORF blockPos) {
		if (blockPos.x >= AppGlobals::CHUNK_WIDTH)
			return true;
		if (blockPos.z >= AppGlobals::CHUNK_WIDTH)
			return true;

		if (blockPos.x < 0)
			return true;
		if (blockPos.y < 0)
			return true;
		if (blockPos.z < 0)
			return true;

		if (blockPos.y >= AppGlobals::CHUNK_HEIGHT) {
			return true;
		}

		return false;
	}
};