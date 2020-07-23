#pragma once
#include "glm.h"
#include "AABB.h"

struct Entity {
	Entity() :
		bbox(glm::vec3(0.f)),
		position(glm::vec3(0.f)),
		rotation(glm::vec3(0.f)),
		velocity(glm::vec3(0.f)){
	}

	Entity(const glm::vec3& pos, const glm::vec3& rot) :
		bbox(glm::vec3(0.f)),
		position(pos),
		rotation(rot),
		velocity(glm::vec3(0.f)) {
	}

	Entity(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& _bbox) :
		bbox(_bbox),
		position(pos),
		rotation(rot),
		velocity(glm::vec3(0.f)) {
	}

	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 velocity;

	AABB bbox;
};