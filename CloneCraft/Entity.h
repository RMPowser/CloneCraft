#pragma once
#include "glm.h"
#include "AABB.h"

struct Entity {
	Entity() :
		bbox(Vec3(0.f)),
		position(glm::vec3(0.f)),
		rotation(Vec3(0.f)),
		velocity(Vec3(0.f)){
	}

	Entity(const glm::vec3& pos, const Vec3& rot) :
		bbox(Vec3(0.f)),
		position(pos),
		rotation(rot),
		velocity(Vec3(0.f)) {
	}

	Entity(const glm::vec3& pos, const Vec3& rot, const Vec3& _bbox) :
		bbox(_bbox),
		position(pos),
		rotation(rot),
		velocity(Vec3(0.f)) {
	}

	glm::vec3 position;
	Vec3 rotation;
	Vec3 velocity;

	AABB bbox;
};