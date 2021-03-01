#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "Layer.hpp"
class Chunk {
public:
	Layer layers[AppGlobals::CHUNK_HEIGHT];
	Vec4 position;
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	bool isInitialized = false;
	bool isLoaded = false;


	Chunk() {}
	Chunk(Vec4 pos) {
		position = Vec4((int)pos.x, 0, (int)pos.z, 0); // chunks never have a y position
	}
	~Chunk() {}

	BlockId getBlock(Vec4 blockPos) {
		if (IsBlockOutOfBounds(blockPos)) {
			return BlockId::Air;
		}

		return layers[(int)blockPos.y].GetBlock(blockPos);
	}

	bool SetBlock(BlockId id, Vec4 blockPos) {
		if (!IsBlockOutOfBounds(blockPos)) {
			if (layers[(int)blockPos.y].SetBlock(id, blockPos)) {
				return true;
			}
		}

		return false;
	}

	bool IsBlockOutOfBounds(Vec4 blockPos) {
		if (blockPos.x < 0 ||
			blockPos.y < 0 ||
			blockPos.z < 0 ||
			blockPos.x >= AppGlobals::CHUNK_WIDTH ||
			blockPos.y >= AppGlobals::CHUNK_HEIGHT ||
			blockPos.z >= AppGlobals::CHUNK_WIDTH) {
			return true;
		}

		return false;
	}


};
#endif // CHUNK_HPP