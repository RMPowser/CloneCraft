#ifndef MATH_HPP
#define MATH_HPP

float PI = 3.14159265358979f;
float RADIAN = PI / 180;

#include "Vector.hpp"
#include "Matrix.hpp"
#include "AABB.hpp"


Vec4 VectorXMatrix(Vec4 v, Mat4 m) {
	GW::MATH::GVECTORF V{ v.x, v.y, v.z, v.w };
	GW::MATH::GMATRIXF M{ 
		m.data[0], m.data[1], m.data[2], m.data[3],
		m.data[4], m.data[5], m.data[6], m.data[7],
		m.data[8], m.data[9], m.data[10], m.data[11],
		m.data[12], m.data[13], m.data[14], m.data[15]
	};

	GW::MATH::GVector::VectorXMatrixF(V, M, V);
	return Vec4(V.x, V.y, V.z, V.w);
}

Vec4 MatrixXVector(Mat4 m, Vec4 v) {
	GW::MATH::GMATRIXF M{
		m.data[0], m.data[1], m.data[2], m.data[3],
		m.data[4], m.data[5], m.data[6], m.data[7],
		m.data[8], m.data[9], m.data[10], m.data[11],
		m.data[12], m.data[13], m.data[14], m.data[15]
	};
	GW::MATH::GVECTORF V{ v.x, v.y, v.z, v.w };

	GW::MATH::GMatrix::VectorXMatrixF(M, V, V);
	return Vec4(V.x, V.y, V.z, V.w);
}

float clamp(float value, float lowerBound, float upperBound) {
	if (value < lowerBound) {
		return lowerBound;
	}
	else if (value > upperBound) {
		return upperBound;
	}
	else {
		return value;
	}
}

#endif // MATH_HPP