#pragma once
class Vec2 {
public:
	float x, y;

	Vec2() {
		x = y = 0;
	}

	Vec2(float x, float y) {
		this->x = x;
		this->y = y;
	}

	Vec2(int x, int y) {
		this->x = (float)x;
		this->y = (float)y;
	}

	Vec2(const Vec2& other) {
		this->operator=(other);
	}

	Vec2 AsInt() {
		return Vec2((int)x, int(y));
	}

	bool operator==(const Vec2& other) {
		return x == other.x && y == other.y;
	}

	bool operator!=(const Vec2& other) {
		return !(*this == other);
	}
};





class Vec4 {
	GW::MATH::GVECTORF GVector(const Vec4& v) const {
		static GW::MATH::GVECTORF r;
		r = { v.x, v.y, v.z, 0 };
		return r;
	}

	GW::MATH::GVECTORF GVector() const {
		return GVector(*this);
	}

public:
	union {
		struct {
			float x, y, z, w;
		};
		float data[4];
	};

	Vec4() {
		x = y = z = w = 0;
	}

	Vec4(float x, float y, float z, float w) {
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}

	Vec4(int x, int y, int z, int w) {
		this->x = (float)x;
		this->y = (float)y;
		this->z = (float)z;
		this->w = (float)w;
	}

	Vec4(const Vec4& other) {
		this->operator=(other);
	}

	float Dot(const Vec4& other) {
		float f;
		GW::MATH::GVector::DotF(GVector(), GVector(other), f);
		return f;
	}

	float Magnitude() const {
		float f;
		GW::MATH::GVector::MagnitudeF(GVector(), f);
		return f;
	}

	Vec4 Cross(const Vec4& other) {
		auto v = GVector();
		GW::MATH::GVector::CrossVector3F(v, GVector(other), v);
		Vec4 r(v.x, v.y, v.z, v.w);
		return r;
	}

	Vec4 Normalized() {
		auto v = GVector();
		GW::MATH::GVector::NormalizeF(v, v);
		Vec4 r(v.x, v.y, v.z, v.w);
		return r;
	}

	Vec4 AsInt() {
		return Vec4((int)x, (int)y, (int)z, (int)w);
	}

	Vec4& operator=(const Vec4& other) {
		x = other.x;
		y = other.y;
		z = other.z;
		w = other.w;

		return *this;
	}

	Vec4& operator+=(const Vec4& other) {
		auto v = GVector();
		GW::MATH::GVector::AddVectorF(v, GVector(other), v);
		x = v.x;
		y = v.y;
		z = v.z;
		w = v.w;
		return *this;
	}

	Vec4& operator-=(const Vec4& other) {
		auto v = GVector();
		GW::MATH::GVector::SubtractVectorF(v, GVector(other), v);
		x = v.x;
		y = v.y;
		z = v.z;
		w = v.w;
		return *this;
	}

	Vec4& operator*=(const float& f) {
		auto v = GVector();
		GW::MATH::GVector::ScaleF(v, f, v);
		x = v.x;
		y = v.y;
		z = v.z;
		w = v.w;
		return *this;
	}

	const Vec4 operator+(const Vec4& other) const {
		return Vec4(*this) += other;
	}

	const Vec4 operator-(const Vec4& other) const {
		return Vec4(*this) -= other;
	}

	const Vec4 operator*(const float f) const {
		return Vec4(*this) *= f;
	}

	bool operator==(const Vec4& other) const {
		return (x == other.x &&
				y == other.y &&
				z == other.z &&
				w == other.w);
	}

	bool operator!=(const Vec4& other) const {
		return !(*this == other);
	}
};