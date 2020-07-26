#pragma once
#include "World.h"
#include "Chunk.h"

std::unordered_map<Vec2XZ, Chunk> worldVector;

World::World(BlockDatabase* _blockdb) {
	blockdb = _blockdb;
}

Vec2XZ World::getBlockXZ(int x, int z) {
	return { x % CHUNK_WIDTH, z % CHUNK_WIDTH };
}

Vec2XZ World::getChunkXZ(int x, int z) {
	return { x / CHUNK_WIDTH, z / CHUNK_WIDTH };
}

BlockId World::getBlock(int x, int y, int z) {
	auto blockPosition = getBlockXZ(x, z);
	auto chunkPosition = getChunkXZ(x, z);

	return getChunk(chunkPosition.x, chunkPosition.z).getBlock(blockPosition.x, y, blockPosition.z);
}

Chunk& World::getChunk(int x, int z) {
	Vec2XZ key{ x, z };
	if (!chunkExistsAt(x, z)) {
		Chunk chunk(this, Vec2XZ{ x, z });
		worldVector.emplace(key, std::move(chunk));
	}

	return worldVector[key];
}

bool World::chunkExistsAt(int x, int z) {
	Vec2XZ key{ x, z };
	return worldVector.find(key) != worldVector.end();
}

bool World::addBlock(BlockId id, int x, int y, int z) {
	auto blockPosition = getBlockXZ(x, z);
	auto chunkPosition = getChunkXZ(x, z);

	if (getChunk(chunkPosition.x, chunkPosition.z).setBlock(id, blockPosition.x, y, blockPosition.z)) {
		return true;
	}

	return false;
}
