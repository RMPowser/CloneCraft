#pragma once
#include "vulkan/vulkan.h"
#include <array>
#include <boost/container_hash/hash.hpp>
#include <cmath>
#include <iostream>

struct Vec3 {
	float x, y, z;

	Vec3() {
		x = 0;
		y = 0;
		z = 0;
	}

	Vec3(float a) {
		x = a;
		y = a;
		z = a;
	}

	Vec3(float a, float b, float c) {
		x = a;
		y = b;
		z = c;
	}

	Vec3(int a, int b, int c) {
		x = a;
		y = b;
		z = c;
	}

	Vec3(const Vec3& other) {
		x = other.x;
		y = other.y;
		z = other.z;
	}



	bool operator==(const Vec3& other) const {
		return x == other.x && y == other.y && z == other.z;
	}

	float& operator[](int i) {
		switch (i) {
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		default:
			__debugbreak();
			throw std::invalid_argument("invalid argument specified for index. valid values are 0, 1, 2");
			break;
		}
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

	Vec3& operator/=(const Vec3& other) {
		this->x /= other.x;
		this->y /= other.y;
		this->z /= other.z;

		return *this;
	}

	Vec3& operator/=(const float& other) {
		this->x /= other;
		this->y /= other;
		this->z /= other;

		return *this;
	}

	Vec3& operator*(const float& other) {
		this->x *= other;
		this->y *= other;
		this->z *= other;

		return *this;
	}

	static float dot(Vec3& a, Vec3& b) {
		float cx = a.x * b.x;
		float cy = a.y * b.y;
		float cz = a.z * b.z;

		return cx + cy + cz;
	}

	static float length(Vec3& vector) {
		return sqrt((vector.x * vector.x) + (vector.y * vector.y) + (vector.z * vector.z));
	}

	static Vec3 normalize(Vec3 vector) {
		float l = length(vector);
		Vec3 result(vector.x / l, vector.y / l, vector.z / l);
		return result;
	}
};

struct Vec4 {
	float x, y, z, w;

	Vec4() {
		x = 0;
		y = 0;
		z = 0;
		w = 0;
	}

	Vec4(float a) {
		x = a;
		y = a;
		z = a;
		w = a;
	}

	Vec4(float a, float b, float c, float d) {
		x = a;
		y = b;
		z = c;
		w = d;
	}

	bool operator==(const Vec4& other) const {
		bool xb = x == other.x;
		bool yb = y == other.y;
		bool zb = z == other.z;
		bool wb = w == other.w;
		return xb && yb && zb && wb;
	}

	float& operator[](int i) {
		switch (i) {
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		case 3:
			return w;
		default:
			__debugbreak();
			throw std::invalid_argument("invalid argument specified for index. valid values are 0, 1, 2, 3");
			break;
		}
	}

	friend std::size_t hash_value(Vec4 const& v) {
		std::size_t seed = 0;
		boost::hash_combine(seed, v.x);
		boost::hash_combine(seed, v.y);
		boost::hash_combine(seed, v.z);
		boost::hash_combine(seed, v.w);

		return seed;
	}

	Vec4& operator+=(const Vec4& other) {
		this->x += other.x;
		this->y += other.y;
		this->z += other.z;
		this->w += other.w;
		return *this;
	}

	Vec4& operator+(const Vec4& other) {
		this->x += other.x;
		this->y += other.y;
		this->z += other.z;
		this->w += other.w;
		return *this;
	}

	Vec4& operator/=(const Vec4& other) {
		this->x /= other.x;
		this->y /= other.y;
		this->z /= other.z;
		this->w /= other.w;
		return *this;
	}

	Vec4& operator/=(const float& other) {
		this->x /= other;
		this->y /= other;
		this->z /= other;
		this->w /= other;
		return *this;
	}

	Vec4& operator*(float& other) {
		this->x *= other;
		this->y *= other;
		this->z *= other;
		this->w *= other;
		return *this;
	}
};

struct Mat4 {
	std::array<Vec4, 4> mat;

	Mat4() {
		mat.fill(Vec4(0.f));
	}

	Mat4(float a) {
		mat.fill(Vec4(a));
	}

	static Mat4 rotate(Mat4& m, float angle, Vec3& v) {
		float a = angle;
		float c = cos(a);
		float s = sin(a);

		Vec3 axis(Vec3::normalize(v));
		Vec3 temp(axis * (float(1) - c));

		Mat4 Rotate;
		Rotate.mat[0][0] = c + temp[0] * axis[0];
		Rotate.mat[0][1] = temp[0] * axis[1] + s * axis[2];
		Rotate.mat[0][2] = temp[0] * axis[2] - s * axis[1];

		Rotate.mat[1][0] = temp[1] * axis[0] - s * axis[2];
		Rotate.mat[1][1] = c + temp[1] * axis[1];
		Rotate.mat[1][2] = temp[1] * axis[2] + s * axis[0];

		Rotate.mat[2][0] = temp[2] * axis[0] + s * axis[1];
		Rotate.mat[2][1] = temp[2] * axis[1] - s * axis[0];
		Rotate.mat[2][2] = c + temp[2] * axis[2];

		Mat4 Result;
		Result.mat[0] = m.mat[0] * Rotate.mat[0][0] + m.mat[1] * Rotate.mat[0][1] + m.mat[2] * Rotate.mat[0][2];
		Result.mat[1] = m.mat[0] * Rotate.mat[1][0] + m.mat[1] * Rotate.mat[1][1] + m.mat[2] * Rotate.mat[1][2];
		Result.mat[2] = m.mat[0] * Rotate.mat[2][0] + m.mat[1] * Rotate.mat[2][1] + m.mat[2] * Rotate.mat[2][2];
		Result.mat[3] = m.mat[3];
		return Result;
	}

	static Mat4 translate(Mat4 m, Vec3 v) {
		Mat4 Result(m);
		Result.mat[3] = m.mat[0] * v[0] + m.mat[1] * v[1] + m.mat[2] * v[2] + m.mat[3];
		return Result;
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
	Vec3 pos = { 0.f, 0.f, 0.f }; // size 12 bytes
	ColorRGB color = { 0.f, 0.f, 0.f }; // size 12 bytes
	Vec2 texCoord = { 0.f, 0.f }; // size 8 bytes

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