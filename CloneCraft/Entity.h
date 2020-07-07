#pragma once
#include "glm.h"

class Entity {
public:
    Entity() {
		position = { 0, 0, 0 };
		rotation = { 0, 0, 0 };
		velocity = { 0, 0, 0 };
    }

	Entity(const glm::vec3& pos, const glm::vec3& rot) {
		position = pos;
		rotation = rot;
		velocity = { 0, 0, 0 };
	}

	~Entity() {};

	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 velocity;
};
