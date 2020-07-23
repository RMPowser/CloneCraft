#include "Player.h"
#include "Block.h"

Player::Player() :
	Entity({ 85.f, 21.f, 14.f }, { 0.f, 0.f, 0.f }, { 0.3f, 1.f, 0.3f }) {
	acceleration = glm::vec3(0.f);
}

void Player::update(float dt) {
	float vMax = 10.f;
	float vMin = 0.05f;
	velocity += acceleration;
	acceleration = { 0, 0, 0 };

	position.x += velocity.x * dt;
	collide(*world, { velocity.x, 0, 0 });

	position.y += velocity.y * dt;
	collide(*world, { 0, velocity.y, 0 });

	position.z += velocity.z * dt;
	collide(*world, { 0, 0, velocity.z });

	velocity.x *= (0.95 * (1 - dt));
	velocity.z *= (0.95 * (1 - dt));
	velocity.y *= (0.95 * (1 - dt));


	if (velocity.x < vMin && velocity.x > -vMin) {
		velocity.x = 0;
	}
	if (velocity.y < vMin && velocity.y > -vMin) {
		velocity.y = 0;
	}
	if (velocity.z < vMin && velocity.z > -vMin) {
		velocity.z = 0;
	}

	bbox.update(position);

	/*float vMax = 10.f;
	float vMin = 0.05f;
	velocity += m_acceleration;
	m_acceleration = glm::vec3(0.f);

	if (velocity.x > vMax) {
		velocity.x = vMax;
	}
	if (velocity.z > vMax) {
		velocity.z = vMax;
	}
	if (velocity.x < -vMax) {
		velocity.x = -vMax;
	}
	if (velocity.z < -vMax) {
		velocity.z = -vMax;
	}
	
	position += velocity * dt;
	collide(*world, velocity);

	velocity *= (0.95 * dt);

	if (velocity.x < vMin && velocity.x > -vMin) {
		velocity.x = 0;
	}
	if (velocity.y < vMin && velocity.y > -vMin) {
		velocity.y = 0;
	}
	if (velocity.z < vMin && velocity.z > -vMin) {
		velocity.z = 0;
	}*/
}

void Player::collide(World& world, const glm::vec3& vel) {
	for (int x = position.x - bbox.dimensions.x; x < position.x + bbox.dimensions.x; x++) {
		for (int y = position.y - bbox.dimensions.y; y < position.y + 0.7; y++) {
			for (int z = position.z - bbox.dimensions.z; z < position.z + bbox.dimensions.z; z++) {
				auto block = world.getBlock(x, y, z);

				bool isCollidable = world.blockdb->blockDataFor(block).isCollidable();

				if (isCollidable) {
					if (vel.y > 0) {
						position.y = y - bbox.dimensions.y;
						//velocity.y = 0;
					} else if (vel.y < 0) {
						//m_isOnGround = true;
						position.y = y + bbox.dimensions.y + 1;
						//velocity.y = 0;
					}

					if (vel.x > 0) {
						position.x = x - bbox.dimensions.x;
					} else if (vel.x < 0) {
						position.x = x + bbox.dimensions.x + 1;
					}

					if (vel.z > 0) {
						position.z = z - bbox.dimensions.z;
					} else if (vel.z < 0) {
						position.z = z + bbox.dimensions.z + 1;
					}
				}
			}
		}
	}
}