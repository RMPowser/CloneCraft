#pragma once
#include "vulkan/vulkan.h"
#include <array>
#include <glm/gtx/hash.hpp>
#include <boost/container_hash/hash.hpp>

struct Vec3 {
	float x, y, z;

	bool operator==(const Vec3& other) const {
		return x == other.x && y == other.y && z == other.z;
	}

	friend std::size_t hash_value(Vec3 const& v) {
		std::size_t seed = 0;
		boost::hash_combine(seed, v.x);
		boost::hash_combine(seed, v.y);
		boost::hash_combine(seed, v.z);

		return seed;
	}

	Vec3& operator+=(const Vec3& other) {
		this->x += other.x;
		this->y += other.y;
		this->z += other.z;
		return *this;
	}
};

struct ColorRGB {
	float r, g, b;

	bool operator==(const ColorRGB& other) const {
		return r == other.r && g == other.g && b == other.b;
	}

	friend std::size_t hash_value(ColorRGB const& v) {
		std::size_t seed = 0;
		boost::hash_combine(seed, v.r);
		boost::hash_combine(seed, v.g);
		boost::hash_combine(seed, v.b);

		return seed;
	}

	ColorRGB& operator+=(const ColorRGB& other) {
		this->r += other.r;
		this->g += other.g;
		this->b += other.b;
		return *this;
	}
};

struct Vec2 {
	float x, y;

	bool operator==(const Vec2& other) const {
		return x == other.x && y == other.y;
	}

	friend std::size_t hash_value(Vec2 const& v) {
		std::size_t seed = 0;
		boost::hash_combine(seed, v.x);
		boost::hash_combine(seed, v.y);
		
		return seed;
	}

	Vec2& operator+=(const Vec2& other) {
		this->x += other.x;
		this->y += other.y;
		return *this;
	}
};

struct Vertex {
	Vec3 pos; // size 12 bytes
	ColorRGB color; // size 12 bytes
	Vec2 texCoord; // size 8 bytes

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};


namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& v) const {
			std::size_t seed = 0;
			boost::hash_combine(seed, v.pos);
			boost::hash_combine(seed, v.color);
			boost::hash_combine(seed, v.texCoord);

			return seed;
		}
	};
}