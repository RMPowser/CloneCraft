#pragma once
#include "Block.h"
#include <vector>
#include <unordered_map>

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

	World(BlockDatabase* _blockdb) {
		blockdb = _blockdb;
	}
	~World() {}

	class Chunk {
	public:
		Chunk() = default;
		Chunk(World* _world, Vec2XZ pos) :
			position(pos),
			world(_world) {
		}
		~Chunk() {}

		BlockId getBlock(int x, int y, int z) {
			if (isBlockOutOfBounds(x, y, z)) {
				return BlockId::Air;
			}

			return chunk[y].getBlock(x, z);
		}

		bool addBlock(BlockId id, int x, int y, int z) {
			if (!isBlockOutOfBounds(x, y, z)) {
				if (chunk[y].addBlock(id, x, z)) {
					return true;
				}
			}

			return false;
		}

		bool isBlockOutOfBounds(int x, int y, int z) {
			if (x >= CHUNK_WIDTH)
				return true;
			if (z >= CHUNK_WIDTH)
				return true;

			if (x < 0)
				return true;
			if (y < 0)
				return true;
			if (z < 0)
				return true;

			if (y >= CHUNK_HEIGHT) {
				return true;
			}

			return false;
		}

		class Layer {
		public:
			Layer() {
				std::array<BlockId, CHUNK_WIDTH> rowOfAir;
				rowOfAir.fill(BlockId::Air);
				layer.fill(rowOfAir);
			}
			~Layer() {}

			BlockId getBlock(int x, int z) {
				return layer[x][z];
			}

			bool addBlock(BlockId id, int x, int z) {
				layer[x][z] = id;
				return true;
			}

			std::array<std::array<BlockId, CHUNK_WIDTH>, CHUNK_WIDTH> layer;
		private:
		};

		std::array<Layer, CHUNK_HEIGHT> chunk;
		const Vec2XZ position;
		const World* world;
	private:
	};


	using WorldVector = std::unordered_map<Vec2XZ, Chunk>;

	static Vec2XZ getBlockXZ(int x, int z) {
		return { x % CHUNK_WIDTH, z % CHUNK_WIDTH };
	}

	static Vec2XZ getChunkXZ(int x, int z) {
		return { x / CHUNK_WIDTH, z / CHUNK_WIDTH };
	}

	BlockId getBlock(int x, int y, int z) {
		auto blockPosition = getBlockXZ(x, z);
		auto chunkPosition = getChunkXZ(x, z);

		return getChunk(chunkPosition.x, chunkPosition.z).getBlock(blockPosition.x, y, blockPosition.z);
	}

	Chunk& getChunk(int x, int z) {
		Vec2XZ key{ x, z };
		if (!chunkExistsAt(x, z)) {
			Chunk chunk(this, Vec2XZ{ x, z });
			worldVector.emplace(key, std::move(chunk));
		}

		return worldVector[key];
	}

	bool chunkExistsAt(int x, int z) {
		Vec2XZ key{ x, z };
		return worldVector.find(key) != worldVector.end();
	}

	bool addBlock(BlockId id, int x, int y, int z) {
		auto blockPosition = getBlockXZ(x, z);
		auto chunkPosition = getChunkXZ(x, z);

		if (getChunk(chunkPosition.x, chunkPosition.z).addBlock(id, blockPosition.x, y, blockPosition.z)) {
			return true;
		}

		return false;
	}

	WorldVector worldVector;

	BlockDatabase* blockdb;
private:
	
};



