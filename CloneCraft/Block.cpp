#include "Block.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

BlockData::BlockData(BlockId _id) {
	id = _id;
	std::string modelPath;
	std::string texturePath;

	switch (id) {
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
		throw std::runtime_error("Failed to create block data: invalid block id.");
		break;
	}

	generateBlockData(modelPath, texturePath);
}

void BlockData::generateBlockData(const std::string& modelPath, const std::string& texturePath) {
	if (modelPath == "" || texturePath == "") {
		return;
	}

	// load model from obj file including all vertices and texcoords. also create unique vertices to prevent duplicates.
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelPath.c_str())) {
		throw std::runtime_error(warn + err);
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	for (auto& shape : shapes) {
		for (auto& index : shape.mesh.indices) {
			Vertex vertex{};

			vertex.pos.x = attrib.vertices[3 * index.vertex_index + 0];
			vertex.pos.y = attrib.vertices[3 * index.vertex_index + 1];
			vertex.pos.z = attrib.vertices[3 * index.vertex_index + 2];

			vertex.color.r = 1.0f;
			vertex.color.g = 1.0f;
			vertex.color.b = 1.0f;

			vertex.texCoord.x = attrib.texcoords[2 * index.texcoord_index + 0];
			vertex.texCoord.y = 1.0f - attrib.texcoords[2 * index.texcoord_index + 1]; // flip the y coord

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}
			indices.push_back(uniqueVertices[vertex]);
		}
	}

	int size = vertices.size();
	

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

	if (!image) {
		throw std::runtime_error("failed to load texture image!");
	}
	texture.width = texWidth;
	texture.height = texHeight;
	texture.numChannels = channelsToLoad;
	texture.size = texWidth * texHeight * channelsToLoad;
	texture.image = image;
}
