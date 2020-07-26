#pragma once
#include "Block.h"
#include <vector>
#include <unordered_map>

class Chunk;

const int CHUNK_WIDTH = 16;
const int CHUNK_HEIGHT = 256;

struct Vec2XZ {
	int x;
	int z;

	bool operator==(const Vec2XZ& other) const {
		return x == other.x && z == other.z;
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
	World(BlockDatabase* _blockdb);
	~World() {}

	static Vec2XZ getBlockXZ(int x, int z);
	static Vec2XZ getChunkXZ(int x, int z);
	BlockId getBlock(int x, int y, int z);
	Chunk& getChunk(int x, int z);
	bool chunkExistsAt(int x, int z);
	bool addBlock(BlockId id, int x, int y, int z);

	// worldVector declared in cpp
	BlockDatabase* blockdb;
private:
	
};



