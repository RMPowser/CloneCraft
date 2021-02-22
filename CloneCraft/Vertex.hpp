#pragma once
#include <boost/container_hash/hash.hpp>

struct Vertex
{
	Vec4 pos = { 0.f, 0.f, 0.f, 0.f }; // size 16 bytes
	Vec4 texCoord = { 0.f, 0.f, 0.f, 0.f }; // size 16 bytes

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const
	{
		return (pos == other.pos &&
				texCoord == other.texCoord);
	}

	bool operator!=(const Vertex& other) const
	{
		return !(*this == other);
	}
};

namespace std
{
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& v) const
		{
			std::size_t seed = 0;
			boost::hash_combine(seed, v.pos.x);
			boost::hash_combine(seed, v.pos.y);
			boost::hash_combine(seed, v.pos.z);
			boost::hash_combine(seed, v.texCoord.x);
			boost::hash_combine(seed, v.texCoord.y);

			return seed;
		}
	};

	template<> struct hash<Vec4>
	{
		size_t operator()(Vec4 const& v) const
		{
			std::size_t seed = 0;
			boost::hash_combine(seed, v.x);
			boost::hash_combine(seed, v.y);
			boost::hash_combine(seed, v.z);
			boost::hash_combine(seed, v.w);

			return seed;
		}
	};
}