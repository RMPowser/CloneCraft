#pragma once
#include "Vertex.hpp"
#include <iostream>
#include <string>
#include <array>
#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

struct BlockTexture {
	uint32_t width = 1;
	uint32_t height = 1;
	uint32_t numChannels = 1;
	uint32_t size = 1;
	unsigned char* image = nullptr;
};

// All the different types of blocks in the game.
enum class BlockId : unsigned char {
	Air = 0,
	Grass = 1,
	
	NUM_TYPES // always leave this as the last enumeration
};

class BlockData {
private:
	BlockId id;
	BlockTexture texture;
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	bool collidable = false;

	void generateBlockData(const std::string& modelPath, const std::string& texturePath)
	{
		if (modelPath == "" || texturePath == "")
		{
			return;
		}

		// load model from obj file including all vertices and texcoords. also create unique vertices to prevent duplicates.
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelPath.c_str()))
		{
			throw std::exception((warn + err).c_str());
		}

		std::unordered_map<Vertex, unsigned int> uniqueVertices{};

		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				Vertex vertex{};

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2],
					0.f
				};

				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
					0.f,
					0.f
				};

				//vertex.color = { 1.0f, 1.0f, 1.0f };

				if (uniqueVertices.find(vertex) == uniqueVertices.end())
				{
					uniqueVertices[vertex] = (int)vertices.size();
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);
			}
		}

		// int size = vertices.size();


		// load texture.
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
		stbi_uc* image = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, channelsToLoad);

		if (!image)
		{
			throw std::exception("failed to load texture image!");
		}
		texture.width = texWidth;
		texture.height = texHeight;
		texture.numChannels = channelsToLoad;
		texture.size = texWidth * texHeight * channelsToLoad;
		texture.image = image;
	}

public:
	BlockData()
	{
		id = BlockId::Air;
	}

	BlockData(BlockId _id)
	{
		id = _id;
		std::string modelPath;
		std::string texturePath;

		switch (id)
		{
			case BlockId::Air:
				modelPath = "";
				texturePath = "";
				collidable = false;
				break;
			case BlockId::Grass:
				modelPath = "models/Block.obj";
				texturePath = "textures/GrassBlock.png";
				collidable = true;
				break;
			default:
				throw std::exception("Failed to create block data: invalid block id.");
				break;
		}

		generateBlockData(modelPath, texturePath);
	}

	~BlockData() {}



	BlockId& getId() { return id; }
	BlockTexture& getTexture() { return texture; }
	std::vector<Vertex>& getVertices() { return vertices; }
	std::vector<unsigned int>& getIndices() { return indices; }
	bool isCollidable() { return collidable; }
};


class BlockDatabase {
public:
	BlockDatabase() {
		for (size_t i = 0; i < static_cast<int>(BlockId::NUM_TYPES); i++) {
			blockDatas[i] = static_cast<BlockId>(i);
		}
	}
	~BlockDatabase() {}

	BlockData& blockDataFor(BlockId id) {
		return blockDatas[static_cast<int>(id)];
	}

private:
	BlockData blockDatas[static_cast<int>(BlockId::NUM_TYPES)];
};