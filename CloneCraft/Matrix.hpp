#pragma once

class Mat4 {
private:
	GW::MATH::GMATRIXF GMatrix(const Mat4& m) const {
		static GW::MATH::GMATRIXF r;
		r.row1 = { m.data[0], m.data[1], m.data[2], m.data[3] };
		r.row2 = { m.data[4], m.data[5], m.data[6], m.data[7] };
		r.row3 = { m.data[8], m.data[9], m.data[10], m.data[11] };
		r.row4 = { m.data[12], m.data[13], m.data[14], m.data[15] };
		return r;
	}

	GW::MATH::GMATRIXF GMatrix() const {
		return GMatrix(*this);
	}

	static Mat4 ToMat4(const GW::MATH::GMATRIXF& m) {
		Mat4 r;
		for (size_t i = 0; i < 16; i++) {
			r.data[i] = m.data[i];
		}

		return r;
	}

public:
	union {
		struct {
			Vec4 row1, row2, row3, row4;
		};
		struct {
			Vec4 right, up, forward, position;
		};
		float data[16];
	};

	static Mat4 IdentityMatrix() {
		Mat4 m(1, 0, 0, 0,
			   0, 1, 0, 0,
			   0, 0, 1, 0,
			   0, 0, 0, 1);
		return m;
	}

	Mat4() {
		for (size_t i = 0; i < 9; i++) {
			data[i] = 0;
		}
	}

	Mat4(const Mat4& other) {
		this->operator=(other);
	}

	Mat4(float x0, float y0, float z0, float w0,
		 float x1, float y1, float z1, float w1,
		 float x2, float y2, float z2, float w2,
		 float x3, float y3, float z3, float w3) 
	{
		row1 = { x0, y0, z0, w0 };
		row2 = { x1, y1, z1, w1 };
		row3 = { x2, y2, z2, w2 };
		row4 = { x3, y3, z3, w3 };
	}

	Mat4 Transposed() const {
		auto m = GMatrix();
		GW::MATH::GMatrix::TransposeF(m, m);
		return ToMat4(m);
	}

	Mat4 Inverted() const {
		auto m = GMatrix();
		GW::MATH::GMatrix::InverseF(m, m);
		return ToMat4(m);
	}

	Mat4& Translate(Vec4 v) {
		auto m = GMatrix();
		GW::MATH::GVECTORF V = { v.x, v.y, v.z, v.w };
		GW::MATH::GMatrix::TranslateLocalF(m, V, m);
		*this = ToMat4(m);
		return *this;
	}

	Mat4& LocalRotateX(float radians) {
		auto m = GMatrix();
		GW::MATH::GMatrix::RotateXLocalF(m, radians, m);
		*this = (ToMat4(m));
		return *this;
	}

	Mat4& GlobalRotateX(float radians) {
		auto m = GMatrix();
		GW::MATH::GMatrix::RotateXGlobalF(m, radians, m);
		*this = (ToMat4(m));
		return *this;
	}

	Mat4& LocalRotateY(float radians) {
		auto m = GMatrix();
		GW::MATH::GMatrix::RotateYLocalF(m, radians, m);
		*this = (ToMat4(m));
		return *this;
	}

	Mat4& GlobalRotateY(float radians) {
		auto m = GMatrix();
		GW::MATH::GMatrix::RotateYGlobalF(m, radians, m);
		*this = (ToMat4(m));
		return *this;
	}

	Mat4& LocalRotateZ(float radians) {
		auto m = GMatrix();
		GW::MATH::GMatrix::RotateZLocalF(m, radians, m);
		*this = (ToMat4(m));
		return *this;
	}

	Mat4& GlobalRotateZ(float radians) {
		auto m = GMatrix();
		GW::MATH::GMatrix::RotateZGlobalF(m, radians, m);
		*this = (ToMat4(m));
		return *this;
	}

	Mat4& operator=(const Mat4& other) {
		for (size_t i = 0; i < 16; i++) {
			data[i] = other.data[i];
		}
		return *this;
	}

	Mat4& operator+=(const Mat4& other) {
		auto m = GMatrix();
		GW::MATH::GMatrix::AddMatrixF(m, GMatrix(other), m);
		*this = (ToMat4(m));
		return *this;
	}

	Mat4& operator-=(const Mat4& other) {
		auto m = GMatrix();
		GW::MATH::GMatrix::SubtractMatrixF(m, GMatrix(other), m);
		*this = (ToMat4(m));
		return *this;
	}

	Mat4& operator*=(const Mat4& other) {
		auto m = GMatrix();
		GW::MATH::GMatrix::MultiplyMatrixF(m, GMatrix(other), m);
		*this = (ToMat4(m));
		return *this;
	}

	Mat4& operator*=(const float& f) {
		auto m = GMatrix();
		GW::MATH::GMatrix::MultiplyNumF(m, f, m);
		*this = (ToMat4(m));
		return *this;
	}

	const Mat4 operator+(const Mat4 other) const {
		return Mat4(*this) += other;
	}

	const Mat4 operator-(const Mat4 other) const {
		return Mat4(*this) -= other;
	}

	const Mat4 operator*(const Mat4 other) const {
		return Mat4(*this) *= other;
	}

	bool operator==(const Mat4 other) const {
		return{
			row1 == other.row1 &&
			row2 == other.row2 &&
			row3 == other.row3 &&
			row4 == other.row4 };
	}

	bool operator!=(const Mat4 other) const {
		return !(*this == other);
	}
};