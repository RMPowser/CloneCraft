#pragma once
#include "Chunk.h"

Chunk::Chunk(World* _world, Vec2XZ pos) :
	position(pos),
	world(_world) {
}

BlockId Chunk::getBlock(int x, int y, int z) {
	if (isBlockOutOfBounds(x, y, z)) {
		return BlockId::Air;
	}

	return layers[y].getBlock(x, z);
}

bool Chunk::setBlock(BlockId id, int x, int y, int z) {
	if (!isBlockOutOfBounds(x, y, z)) {
		if (layers[y].setBlock(id, x, z)) {
			return true;
		}
	}

	return false;
}

bool Chunk::isBlockOutOfBounds(int x, int y, int z) {
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

void Chunk::generateVerticesAndIndices() {
	vertices.clear();
	indices.clear();
	for (int y = 0; y < CHUNK_HEIGHT; y++) {
		for (int x = 0; x < CHUNK_WIDTH; x++) {
			for (int z = 0; z < CHUNK_WIDTH; z++) {
				// for each block in this chunk
				auto blockId = getBlock(x, y, z);

				if (blockId == BlockId::Air) {
					continue; // dont render air
				}
				
				// infer the block position using its coordinates
				Vec3 blockPosition = { x, y, z };

				// get its data
				auto verts = world->blockdb->blockDataFor(blockId).getVertices();
				auto inds = world->blockdb->blockDataFor(blockId).getIndices();

				// account for the block position and store the new verts
				for (int i = 0; i < verts.size(); i++) {
					Vertex v(verts[i]);
					v.pos += blockPosition;
					vertices.push_back(v);
				}

				// store the indices for later accounting for the offset into the verts vector
				for (int i = 0; i < inds.size(); i++) {
					int ind(inds[i] + vertices.size());
					indices.push_back(ind);
				}
			}
		}
	}
}

void Chunk::load() {
	if (isLoaded) {
		return;
	}

	// todo: actual terrain generation
	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < CHUNK_WIDTH; x++) {
			for (int z = 0; z < CHUNK_WIDTH; z++) {
				setBlock(BlockId::Grass, x, y, z);
			}
		}
	}

	isLoaded = true;
}
