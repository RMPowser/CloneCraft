#pragma once
#include "Layer.h"

Layer::Layer() {
	std::array<BlockId, CHUNK_WIDTH> rowOfAir;
	rowOfAir.fill(BlockId::Air);
	blocks.fill(rowOfAir);
}

BlockId Layer::getBlock(int x, int z) {
	return blocks[x][z];
}

bool Layer::setBlock(BlockId id, int x, int z) {
	blocks[x][z] = id;
	return true;
}
