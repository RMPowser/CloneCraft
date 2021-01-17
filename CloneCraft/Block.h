#pragma once
#include "Vertex.h"
#include <iostream>
#include <string>
#include <array>
#include <unordered_map>

struct BlockTexture {
	uint32_t width = 1;
	uint32_t height = 1;
	uint32_t numChannels = 1;
	uint32_t size = 1;
	unsigned char* image = nullptr;
};


/// <summary>
/// All the different types of blocks in the game.
/// </summary>
enum class BlockId : uint8_t {
	Air = 0,
	Grass = 1,
	
	NUM_TYPES // always leave this as the last enumeration
};

class BlockData {
public:
	BlockData(BlockId _id);
	~BlockData() {}

	BlockId& getId() { return id; }
	BlockTexture& getTexture() { return texture; }
	std::vector<Vertex>& getVertices() { return vertices; }
	std::vector<unsigned int>& getIndices() { return indices; }
	bool isCollidable() { return collidable; }

private:
	BlockId id = BlockId::Air;
	BlockTexture texture;
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	bool collidable = false;

	void generateBlockData(const std::string& modelPath, const std::string& texturePath);
};


class BlockDatabase {
public:
	BlockDatabase() {}
	~BlockDatabase() {}

	BlockData& blockDataFor(BlockId id) {
		return blockDatas[(int)id];
	}

private:
	BlockData blockDatas[(int)BlockId::NUM_TYPES] = {	BlockData(BlockId::Air), 
														BlockData(BlockId::Grass) };
};