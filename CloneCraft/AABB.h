#pragma once
#include "Vertex.h"

/// <summary>
/// Axis-Aligned Bounding Box. Used for collision detection.
/// </summary>
class AABB {
public:
	AABB(Vec3 dim) {
        dimensions.x = dim.x;
        dimensions.y = dim.y;
        dimensions.z = dim.z;
		position = { 0, 0, 0 };
	}
	
	~AABB() {}

	void update(glm::vec3 pos) {
		position.x = pos.x;
        position.y = pos.y;
        position.z = pos.z;
	}

    Vec3 getVNegative(Vec3& normal) {
        Vec3 res = position;

        if (normal.x < 0) {
            res.x += dimensions.x;
        }
        if (normal.y < 0) {
            res.y += dimensions.y;
        }
        if (normal.z < 0) {
            res.z += dimensions.z;
        }

        return res;
    }

    Vec3 getVPositive(Vec3& normal) {
        Vec3 res = position;

        if (normal.x > 0) {
            res.x += dimensions.x;
        }
        if (normal.y > 0) {
            res.y += dimensions.y;
        }
        if (normal.z > 0) {
            res.z += dimensions.z;
        }

        return res;
    }

	Vec3 position;
	Vec3 dimensions;
};

