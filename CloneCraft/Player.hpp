#pragma once
#include "Entity.hpp"
#include "Ray.hpp"
#include <vector>

class Player : public Entity {
	bool flyToggle = true; // indicates weather flying can be toggled or not. ie: stops the game from spam-toggling flying when you press F
	float fallingSpeed = 0; // determines vertical movement

public:
	bool isFlying = false;
	bool onGround = false;
	bool canJump = true;
	bool canPlaceBlock = true;
	bool canBreakBlock = true;
	bool lastFlyingState = false;

	Player() {
		position = { 1000, 130, 1000, 0 }; // set spawn
		bbox.dimensions = { width, height, width, 0.0f };
	}

	void update(float dt, Entity& camera) {
		Window& window = AppGlobals::window;
		auto& controller = AppGlobals::controller;
		auto jumpHeight = AppGlobals::jumpHeight;
		auto buildRange = AppGlobals::buildRange;
		auto speed = AppGlobals::playerSpeed;
		auto gravity = AppGlobals::gravity;


		// --- horizontal movement --- //
		Vec4 moveDirection = GetRightAxis() * controller.horizontalAxis + GetForwardAxis() * controller.forwardAxis;
		moveDirection = moveDirection.Normalized();

		if (controller.keys[G_KEY_LEFTSHIFT]) {
			speed = AppGlobals::playerSpeed * 1.5f;
		}
		else {
			speed = AppGlobals::playerSpeed;
		}

		position.x += moveDirection.x * speed * dt;
		collide(Vec4(moveDirection.x, 0.f, 0.f, 0.f));
		position.z += moveDirection.z * speed * dt;
		collide(Vec4(0.f, 0.f, moveDirection.z, 0.f));


		// toggle flying
		if (controller.keys[G_KEY_F] && flyToggle) {
			isFlying = !isFlying;
			flyToggle = false;
		}
		else if (!controller.keys[G_KEY_F]) {
			flyToggle = true;
		}


		// place block
		if (controller.keys[G_BUTTON_LEFT] && canPlaceBlock) {
			auto& world = AppGlobals::world;

			for (Ray ray(camera.position, camera.rotation); ray.GetLength() <= buildRange; ray.Step(0.001f)) {
				auto rayEnd = ray.GetEnd();
				auto block = world.getBlock(rayEnd);

				if (block != BlockId::Air) {
					if (world.setBlock(BlockId::Air, rayEnd)) {
						Vec4 xz = World::getChunkXZ(rayEnd);
						world.updateChunk(world.getChunk(xz)->position);
						break;
					}
					else {
						__debugbreak();
						throw new std::runtime_error("unable to destroy block!");
					}
				}
			}

			canPlaceBlock = false;
		}
		else if (!controller.keys[G_BUTTON_LEFT]) {
			canPlaceBlock = true;
		}


		// break block
		if (controller.keys[G_BUTTON_RIGHT] && canBreakBlock) {
			Vec4 lastRayPosition{ 0, 0, 0, 0 };
			auto& world = AppGlobals::world;

			for (Ray ray(camera.position, camera.rotation); ray.GetLength() <= buildRange; ray.Step(0.001f)) {
				auto rayEnd = ray.GetEnd();
				auto block = world.getBlock(rayEnd);
				auto blockPosition = lastRayPosition.AsInt();

				if (block != BlockId::Air) {
					if (!wouldCollide(blockPosition)) {
						if (world.setBlock(BlockId::Grass, lastRayPosition)) {
							Vec4 xz = World::getChunkXZ(lastRayPosition);
							world.updateChunk(world.getChunk(xz)->position);
							break;
						}
					}
					else {
						break;
					}
				}
				lastRayPosition = ray.GetEnd();
			}
			canBreakBlock = false;
		}
		else if (!controller.keys[G_BUTTON_RIGHT]) {
			canBreakBlock = true;
		}


		// --- mouse look --- //
		float dx = controller.mouseDelta.x;
		float dy = controller.mouseDelta.y;

		rotation.y += dx * AppGlobals::mouseSensitivity;
		camera.rotation.x += dy * AppGlobals::mouseSensitivity;

		if (camera.rotation.x > AppGlobals::mouseBound)
			camera.rotation.x = AppGlobals::mouseBound;
		else if (camera.rotation.x < -AppGlobals::mouseBound)
			camera.rotation.x = -AppGlobals::mouseBound;

		if (rotation.y > 360)
			rotation.y = 0;
		else if (rotation.y < 0)
			rotation.y = 360;


		if (onGround) {
			fallingSpeed = 0;
		}


		// --- jumping --- //
		if (controller.keys[G_KEY_SPACE] && canJump && !isFlying && onGround) {
			// fall up lol
			fallingSpeed = sqrtf(jumpHeight * -2.f * gravity);
			canJump = false;
		}
		else if (!controller.keys[G_KEY_SPACE]) {
			canJump = true;
		}


		// --- gravity --- //
		if (!isFlying) {
			fallingSpeed += gravity * dt;
		}
		else {
			fallingSpeed = controller.verticalAxis * speed;
		}


		// --- world wrap --- //
		if (position.y < 0 && !isFlying) {
			position.y = 260;
		}

		// collide() will set onGround
		onGround = false;


		
		position.y += fallingSpeed * dt;
		collide(Vec4(0.f, fallingSpeed, 0.f, 0.f));

		bbox.position = position;
	}


	void collide(const Vec4& vel) {
		float xMin = position.x - bbox.dimensions.x;
		float xMax = position.x + bbox.dimensions.x;
		float yMin = position.y;
		float yMax = position.y + bbox.dimensions.y; // "height" blocks above us
		float zMin = position.z - bbox.dimensions.z;
		float zMax = position.z + bbox.dimensions.z;
		auto& world = AppGlobals::world;

		for (int x = xMin; x < xMax; x++) {
			for (int y = yMin; y < yMax; y++) {
				for (int z = zMin; z < zMax; z++) {
					auto block = world.getBlock(Vec4(x, y, z, 0));

					if (world.blockdb.blockDataFor(block).isCollidable()) {
						if (vel.y > 0) {
							position.y = y - bbox.dimensions.y;
						}
						else if (vel.y < 0) {
							onGround = true;
							position.y = y + 1;
						}

						if (vel.x > 0) {
							position.x = x - bbox.dimensions.x;
						}
						else if (vel.x < 0) {
							position.x = x + bbox.dimensions.x + 1;
						}

						if (vel.z > 0) {
							position.z = z - bbox.dimensions.z;
						}
						else if (vel.z < 0) {
							position.z = z + bbox.dimensions.z + 1;
						}
					}
				}
			}
		}
	}

	bool wouldCollide(Vec4 blockPosition) {
		float xMin = position.x - bbox.dimensions.x;
		float xMax = position.x + bbox.dimensions.x;
		float yMin = position.y;
		float yMax = position.y + bbox.dimensions.y; // `height` blocks above us
		float zMin = position.z - bbox.dimensions.z;
		float zMax = position.z + bbox.dimensions.z;

		for (int x = xMin; x < xMax; x++) {
			for (int y = yMin; y < yMax; y++) {
				for (int z = zMin; z < zMax; z++) {
					if (blockPosition == Vec4(x, y, z, 0 )) {
						return true;
					}
				}
			}
		}
		return false;
	}
};