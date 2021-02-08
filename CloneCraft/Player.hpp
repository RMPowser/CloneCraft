#pragma once
#include "Entity.hpp"
#include "Vertex.hpp"
#include "Controller.hpp"
#include <vector>

class Player : public Entity {
public:
	GW::MATH::GVECTORF acceleration;
	bool isOnGround;
	bool isFlying;
	float height = 1.75f;
	float width = 0.25f;

	Player() {
		position = { 1000, 130, 1000, 0 };
		rotation = { 0, 0, 0, 0 };
		velocity = GW::MATH::GZeroVectorF;
		acceleration = GW::MATH::GZeroVectorF;
		updateBbox();
	}

	void update(float dt, Controller controller) {
		GW::MATH::GVector::AddVectorF(velocity, acceleration, velocity);
		acceleration = GW::MATH::GZeroVectorF;

		float vMax = 0.5f;
		float vMin = 0.05f;

		float maxRunningSpeed = 10.0f;

		if (!isFlying) {
			if (!isOnGround) {
				velocity.y -= 50.f * dt;
			}
		}

		if (position.y <= 0 && !isFlying) {
			position.y = 260;
		}

		position.x += velocity.x * dt;
		collide({ velocity.x, 0, 0, 0 });

		position.y += velocity.y * dt;
		collide({ 0, velocity.y, 0, 0 });

		position.z += velocity.z * dt;
		collide({ 0, 0, velocity.z, 0 });

		velocity.x *= (0.75 * (1 - dt));
		velocity.z *= (0.75 * (1 - dt));
		if (isFlying) {
			velocity.y *= (0.75 * (1 - dt));
		}

		if (velocity.x < vMin && velocity.x > -vMin) {
			velocity.x = 0;
		}
		if (velocity.z < vMin && velocity.z > -vMin) {
			velocity.z = 0;
		}

		if (velocity.y > 0) {
			isOnGround = false;
		}

		updateBbox();
	}

	void updateBbox() {
		bbox.min = { position.x, position.y, position.z, 0.0f }; // min is used as position of bbox
		bbox.max = { width, height, width, 0.0f }; // max is used as dimensions of bbox
	}

	void collide(const GW::MATH::GVECTORF& vel) {
		float xMin = position.x - bbox.max.x;
		float xMax = position.x + bbox.max.x;
		float yMin = position.y - bbox.max.y;
		float yMax = position.y + 0.2f;
		float zMin = position.z - bbox.max.z;
		float zMax = position.z + bbox.max.z;

		for (int x = xMin; x < xMax; x++) {
			for (int y = yMin; y < yMax; y++) {
				for (int z = zMin; z < zMax; z++) {
					auto block = AppGlobals::world.getBlock({ static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), 0 });


					bool isCollidable = AppGlobals::world.blockdb.blockDataFor(block).isCollidable();

					if (isCollidable) {
						if (vel.y > 0) {
							position.y = y - 0.2f;
							velocity.y = 0;
						}
						else if (vel.y < 0) {
							isOnGround = true;
							position.y = y + bbox.max.y + 1;
							velocity.y = 0;
						}

						if (vel.x > 0) {
							position.x = x - bbox.max.x;
						}
						else if (vel.x < 0) {
							position.x = x + bbox.max.x + 1;
						}

						if (vel.z > 0) {
							position.z = z - bbox.max.z;
						}
						else if (vel.z < 0) {
							position.z = z + bbox.max.z + 1;
						}
					}
				}
			}
		}
	}
};