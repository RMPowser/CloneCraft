#pragma once
#include "glm.h"

/// <summary>
/// Axis-Aligned Bounding Box. Used for collision detection.
/// </summary>
class AABB {
public:
	AABB(glm::vec3 dim) :
		dimensions(dim) {
		position = { 0, 0, 0 };
	}
	
	~AABB() {}

	void update(glm::vec3 pos) {
		position = pos;
	}

	glm::vec3 position;
	glm::vec3 dimensions;
};

