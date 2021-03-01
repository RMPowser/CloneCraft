#ifndef BLOCK_HPP
#define BLOCK_HPP


#include "Vertex.hpp"
#include <iostream>
#include <string>
#include <array>
#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// All the different types of blocks in the game.
enum class BlockType : unsigned char {
	Air = 0,
	Grass = 1,

	NUM_TYPES // always leave this as the last enumeration
};

class BlockTextureAtlas {
private:
	uint32_t width = 1;
	uint32_t height = 1;
	uint32_t numChannels = 1;
	uint32_t size = 1;
	unsigned char* image = nullptr;

public:
	BlockTextureAtlas() {
		// load texture atlas.
		// The stbi_load function takes the file path and number of channels to load as arguments. 
		// The STBI_rgb_alpha value forces the image to be loaded with an alpha channel, even if it 
		// doesn't have one, which is nice for consistency with other textures in the future. The 
		// middle three parameters are outputs for the width, height and actual number of channels in the image. 
		// The pointer that is returned is the first element in an array of pixel values. The pixels are 
		// laid out row by row with 4 bytes per pixel in the case of STBI_rgb_alpha for a total of texWidth * texHeight * 4 values.

		int texWidth;
		int texHeight;
		int texChannels;
		int channelsToLoad = STBI_rgb_alpha;
		stbi_uc* stbi_image = stbi_load("textures/textureAtlas.png", &texWidth, &texHeight, &texChannels, channelsToLoad);

		if (!stbi_image)
			throw std::exception("failed to load top texture image!");

		width = texWidth;
		height = texHeight;
		numChannels = channelsToLoad;
		size = texWidth * texHeight * channelsToLoad;
		image = stbi_image;
	}

	uint32_t GetWidth() {
		return width;
	}

	uint32_t GetHeight() {
		return height;
	}

	uint32_t GetNumChannels() {
		return numChannels;
	}

	uint32_t GetSize() {
		return size;
	}

	unsigned char* GetPixels() {
		return image;
	}
};

struct BlockFace {
	Vertex vertices[4];
	unsigned int indices[6];
};

enum class BlockFaces : unsigned char {
	north,
	south,
	east,
	west,
	top,
	bottom
};

class BlockData {
private:
	BlockType id;
	BlockFace faces[6];
	bool collidable = false;

	void GenerateBlockData(BlockType id) {
		// textureAtlas is 256x256 blocks
		float unit = 1.0 / 256.0;

		faces[(int)BlockFaces::north].vertices[0].pos = { 1, 1, 1, 0 };
		faces[(int)BlockFaces::north].vertices[1].pos = { 0, 1, 1, 0 };
		faces[(int)BlockFaces::north].vertices[2].pos = { 0, 0, 1, 0 };
		faces[(int)BlockFaces::north].vertices[3].pos = { 1, 0, 1, 0 };
		faces[(int)BlockFaces::north].vertices[0].texCoord = { 2 * unit, 0 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::north].vertices[1].texCoord = { 1 * unit, 0 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::north].vertices[2].texCoord = { 1 * unit, 1 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::north].vertices[3].texCoord = { 2 * unit, 1 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::north].indices[0] = 1;
		faces[(int)BlockFaces::north].indices[1] = 0;
		faces[(int)BlockFaces::north].indices[2] = 3;
		faces[(int)BlockFaces::north].indices[3] = 1;
		faces[(int)BlockFaces::north].indices[4] = 3;
		faces[(int)BlockFaces::north].indices[5] = 2;

		faces[(int)BlockFaces::south].vertices[0].pos = { 1, 1, 0, 0 };
		faces[(int)BlockFaces::south].vertices[1].pos = { 0, 1, 0, 0 };
		faces[(int)BlockFaces::south].vertices[2].pos = { 0, 0, 0, 0 };
		faces[(int)BlockFaces::south].vertices[3].pos = { 1, 0, 0, 0 };
		faces[(int)BlockFaces::south].vertices[0].texCoord = { 1 * unit, 0 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::south].vertices[1].texCoord = { 2 * unit, 0 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::south].vertices[2].texCoord = { 2 * unit, 1 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::south].vertices[3].texCoord = { 1 * unit, 1 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::south].indices[0] = 0;
		faces[(int)BlockFaces::south].indices[1] = 1;
		faces[(int)BlockFaces::south].indices[2] = 3;
		faces[(int)BlockFaces::south].indices[3] = 1;
		faces[(int)BlockFaces::south].indices[4] = 2;
		faces[(int)BlockFaces::south].indices[5] = 3;

		faces[(int)BlockFaces::east].vertices[0].pos = { 1, 1, 1, 0 };
		faces[(int)BlockFaces::east].vertices[1].pos = { 1, 1, 0, 0 };
		faces[(int)BlockFaces::east].vertices[2].pos = { 1, 0, 0, 0 };
		faces[(int)BlockFaces::east].vertices[3].pos = { 1, 0, 1, 0 };
		faces[(int)BlockFaces::east].vertices[0].texCoord = { 1 * unit, 0 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::east].vertices[1].texCoord = { 2 * unit, 0 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::east].vertices[2].texCoord = { 2 * unit, 1 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::east].vertices[3].texCoord = { 1 * unit, 1 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::east].indices[0] = 0;
		faces[(int)BlockFaces::east].indices[1] = 1;
		faces[(int)BlockFaces::east].indices[2] = 3;
		faces[(int)BlockFaces::east].indices[3] = 1;
		faces[(int)BlockFaces::east].indices[4] = 2;
		faces[(int)BlockFaces::east].indices[5] = 3;

		faces[(int)BlockFaces::west].vertices[0].pos = { 0, 1, 0, 0 };
		faces[(int)BlockFaces::west].vertices[1].pos = { 0, 1, 1, 0 };
		faces[(int)BlockFaces::west].vertices[2].pos = { 0, 0, 1, 0 };
		faces[(int)BlockFaces::west].vertices[3].pos = { 0, 0, 0, 0 };
		faces[(int)BlockFaces::west].vertices[0].texCoord = { 1 * unit, 0 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::west].vertices[1].texCoord = { 2 * unit, 0 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::west].vertices[2].texCoord = { 2 * unit, 1 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::west].vertices[3].texCoord = { 1 * unit, 1 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::west].indices[0] = 0;
		faces[(int)BlockFaces::west].indices[1] = 1;
		faces[(int)BlockFaces::west].indices[2] = 3;
		faces[(int)BlockFaces::west].indices[3] = 1;
		faces[(int)BlockFaces::west].indices[4] = 2;
		faces[(int)BlockFaces::west].indices[5] = 3;

		faces[(int)BlockFaces::top].vertices[0].pos = { 0, 1, 0, 0 };
		faces[(int)BlockFaces::top].vertices[1].pos = { 1, 1, 0, 0 };
		faces[(int)BlockFaces::top].vertices[2].pos = { 1, 1, 1, 0 };
		faces[(int)BlockFaces::top].vertices[3].pos = { 0, 1, 1, 0 };
		faces[(int)BlockFaces::top].vertices[0].texCoord = { 0 * unit, 0 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::top].vertices[1].texCoord = { 1 * unit, 0 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::top].vertices[2].texCoord = { 1 * unit, 1 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::top].vertices[3].texCoord = { 0 * unit, 1 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::top].indices[0] = 0;
		faces[(int)BlockFaces::top].indices[1] = 1;
		faces[(int)BlockFaces::top].indices[2] = 3;
		faces[(int)BlockFaces::top].indices[3] = 1;
		faces[(int)BlockFaces::top].indices[4] = 2;
		faces[(int)BlockFaces::top].indices[5] = 3;

		faces[(int)BlockFaces::bottom].vertices[0].pos = { 0, 0, 1, 0 };
		faces[(int)BlockFaces::bottom].vertices[1].pos = { 1, 0, 1, 0 };
		faces[(int)BlockFaces::bottom].vertices[2].pos = { 1, 0, 0, 0 };
		faces[(int)BlockFaces::bottom].vertices[3].pos = { 0, 0, 0, 0 };
		faces[(int)BlockFaces::bottom].vertices[0].texCoord = { 2 * unit, 0 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::bottom].vertices[1].texCoord = { 3 * unit, 0 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::bottom].vertices[2].texCoord = { 3 * unit, 1 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::bottom].vertices[3].texCoord = { 2 * unit, 1 * unit, 0 * unit, 0 * unit };
		faces[(int)BlockFaces::bottom].indices[0] = 0;
		faces[(int)BlockFaces::bottom].indices[1] = 1;
		faces[(int)BlockFaces::bottom].indices[2] = 3;
		faces[(int)BlockFaces::bottom].indices[3] = 1;
		faces[(int)BlockFaces::bottom].indices[4] = 2;
		faces[(int)BlockFaces::bottom].indices[5] = 3;
	}

public:
	BlockData() {
		id = BlockType::Air;
	}

	BlockData(BlockType _id) {
		id = _id;

		switch (id) {
			case BlockType::Air:
				collidable = false;
				break;
			case BlockType::Grass:
				collidable = true;
				break;
			default:
				throw std::exception("Failed to create block data: invalid block id.");
				break;
		}

		GenerateBlockData(id);
	}

	~BlockData() {}



	BlockType getId() { return id; }
	Vertex* getFaceVertices(BlockFaces face) { return faces[(int)face].vertices; }
	unsigned int* getFaceIndices(BlockFaces face) { return faces[(int)face].indices; }
	bool isCollidable() { return collidable; }
};


class BlockDatabase {
private:
	BlockTextureAtlas textureAtlas;
	BlockData blockDatas[(int)(BlockType::NUM_TYPES)];

public:
	BlockDatabase() {
		for (size_t i = 0; i < (unsigned int)(BlockType::NUM_TYPES); i++) {
			blockDatas[i] = (BlockType)i;
		}
	}
	~BlockDatabase() {}

	BlockData GetBlockDataFor(BlockType id) {
		return blockDatas[(int)id];
	}

	BlockTextureAtlas GetTextureAtlas() {
		return textureAtlas;
	}
};
#endif // BLOCK_HPP