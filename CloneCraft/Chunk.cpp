#pragma once
#include "Chunk.h"

Chunk::Chunk() : 
	bbox({ CHUNK_WIDTH, CHUNK_HEIGHT, CHUNK_WIDTH }) {
}

Chunk::Chunk(World& _world, Vec2XZ pos) :
	position(pos),
	world(&_world),
	bbox({ CHUNK_WIDTH, CHUNK_HEIGHT, CHUNK_WIDTH }) {
	bbox.update({ pos.x * CHUNK_WIDTH, 0, pos.z * CHUNK_WIDTH }); // bbox position is in world coordinates, not chunk coodinates
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
	for (int y = 0; y < CHUNK_HEIGHT; y++) {
		for (int x = 0; x < CHUNK_WIDTH; x++) {
			for (int z = 0; z < CHUNK_WIDTH; z++) {
				// for each block in this chunk
				auto blockId = getBlock(x, y, z);

				// infer the block position using its coordinates
				Vec3 blockPosition = { x, y, z };

				// dont render air
				if (blockId == BlockId::Air) {
					continue;
				}

				// get the block's data
				auto verts = world->blockdb->blockDataFor(blockId).getVertices();
				auto inds = world->blockdb->blockDataFor(blockId).getIndices();

				// save the offset for the indices
				auto offset = vertices.size();

				// account for the block position and chunk position and store the new verts for later
				for (int i = 0; i < verts.size(); i++) {
					Vertex v(verts[i]);
					v.pos += blockPosition;
					v.pos.x += position.x * CHUNK_WIDTH; // coords are now in world coords format. 
					v.pos.z += position.z * CHUNK_WIDTH;
					vertices.push_back(v);
				}

				// account for the offset into vertices vector and store the indices for later
				for (int i = 0; i < inds.size(); i++) {
					auto ind(inds[i] + offset);
					indices.push_back(ind);
				}
			}
		}
	}
}