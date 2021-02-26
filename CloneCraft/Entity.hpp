#ifndef ENTITY_HPP
#define ENTITY_HPP


class Entity {
public:
	Vec4 position = { 0, 0, 0, 0 };
	Vec4 rotation = { 0, 0, 0, 0 }; // the forward direction expressed as an angle of rotation around each axis
	AABB bbox = AABB(Vec4(0, 0, 0, 0), Vec4(0, 0, 0, 0));
	float height = 1.75f;
	float width = 0.25f;

	Entity() {}

	Entity(Vec4 pos) {
		position = pos;
	}

	Entity(Vec4 pos, Vec4 rot) {
		position = pos;
		rotation = rot;
	}

	Entity(Vec4 pos, Vec4 rot, AABB box) {
		position = pos;
		rotation = rot;
		bbox = box;
	}

	Vec4 GetForwardAxis() {
		float a = rotation.y * RADIAN;
		Vec4 r{ 0, 0, 1, 0 }; // +z axis is forward

		Mat4 rotateY = {	 cos(a),	 0,	 sin(a),     0,
								  0,	 1,		  0,     0,
							-sin(a),	 0,	 cos(a),     0,
								  0,     0,       0,     1 };

		r = VectorXMatrix(r, rotateY);
		return r;
	}

	Vec4 GetRightAxis() {
		Vec4 forwardAxis = GetForwardAxis();
		return forwardAxis.Cross(Vec4(0, 1, 0, 0));
	}

	Vec4 GetUpAxis() {
		Vec4 forwardAxis = GetForwardAxis();
		Vec4 rightAxis = forwardAxis.Cross(Vec4(0, 1, 0, 0));
		return forwardAxis.Cross(rightAxis);
	}

	bool operator==(const Entity& other) const {
		return (position == other.position &&
				bbox == other.bbox &&
				height == other.height &&
				width == other.width);
	}

	bool operator!=(const Entity& other) const {
		return !(*this == other);
	}
};
#endif // ENTITY_HPP