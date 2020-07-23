#pragma once
#include "Vertex.h"
#include <iostream>
#include <string>
#include <array>
#include <unordered_map>

struct BlockTexture {
	int width = 1;
	int height = 1;
	int numChannels = 1;
	int size = 1;
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
	std::vector<Vertex>& getVertices() {
		return vertices; 
	}
	std::vector<unsigned int>& getIndices() { return indices; }
	bool isCollidable() { return collidable; }

private:
	BlockId id;
	BlockTexture texture;
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	bool collidable;

	void generateBlockData(const std::string& modelPath, const std::string& texturePath);
};


class BlockDatabase {
public:
	BlockDatabase() {
		blockDatas[0] = new BlockData(BlockId::Air);
		blockDatas[1] = new BlockData(BlockId::Grass);
	}
	~BlockDatabase() {
		for (auto& blockData : blockDatas) {
			delete blockData;
		}
	}

	BlockData& blockDataFor(BlockId id) {
		return *blockDatas[(int)id];
	}

private:
	std::array<BlockData*, (int)BlockId::NUM_TYPES> blockDatas;
};