#include "Player.h"

Player::Player()
{
	position = { 0, 0, 2 };
	velocity = { 0.f, 0.f, 0.f };
	m_acceleration = glm::vec3(0.f);
}

void Player::update(float dt) {
	float velocityLimit = 1;
	velocity += m_acceleration;
	m_acceleration = { 0, 0, 0 };

	//if (velocity.x > velocityLimit) {
	//	velocity.x = velocityLimit;
	//}
	//if (velocity.z > velocityLimit) {
	//	velocity.z = velocityLimit;
	//}
	//if (velocity.x < -velocityLimit) {
	//	velocity.x = -velocityLimit;
	//}
	//if (velocity.z < -velocityLimit) {
	//	velocity.z = -velocityLimit;
	//}d

	position += velocity * dt;
   
	velocity *= 0.95;

	if (velocity.x < 0.01 && velocity.x > -0.01) {
		velocity.x = 0;
	}
	if (velocity.y < 0.01 && velocity.y > -0.01) {
		velocity.y = 0;
	}
	if (velocity.z < 0.01 && velocity.z > -0.01) {
		velocity.z = 0;
	}
}