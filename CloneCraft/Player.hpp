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

    Player() :
        Entity({ 1000, 130, 1000, 0 }, { 0, 0, 0, 0 }, { 0.3f, 1.0f, 0.3f, 0.0f, 0.5f, 1.5f, 0.5f, 0.0f })
    {
        acceleration = GW::MATH::GZeroVectorF;
    }

	void update(float dt, Controller controller)
	{
		GW::MATH::GVector::AddVectorF(velocity, acceleration, velocity);
		acceleration = GW::MATH::GZeroVectorF;

		float velocityMax = 10;

		if (!isFlying)
		{
			if (!isOnGround)
			{
				velocity.y -= 50.f * dt;
			}
			isOnGround = false;
		}

		if (position.y <= 0 && !isFlying)
		{
			position.y = 260;
		}

		position.x += velocity.x * dt;
		collide({ velocity.x, 0, 0, 0 });

		position.y += velocity.y * dt;
		collide({ 0, velocity.y, 0, 0 });

		position.z += velocity.z * dt;
		collide({ 0, 0, velocity.z, 0 });

		velocity.x *= (0.95f * (1.0f - dt));
		velocity.z *= (0.95f * (1.0f - dt));
		if (isFlying)
		{
			velocity.y *= (0.95f * (1.0f - dt));
		}

		if (velocity.x > velocityMax) { velocity.x = velocityMax; }
		else if (velocity.x < -velocityMax) { velocity.x = -velocityMax; }
		if (velocity.y > velocityMax) { velocity.y = velocityMax; }
		else if (velocity.y < -velocityMax) { velocity.y = -velocityMax; }
		if (velocity.z > velocityMax) { velocity.z = velocityMax; }
		else if (velocity.z < -velocityMax) { velocity.z = -velocityMax; }

		bbox.center = position;
	}

	void collide(const GW::MATH::GVECTORF& vel)
	{
		for (float x = position.x - bbox.extent.x; x < position.x + bbox.extent.x; x++)
		{
			for (float y = position.y - bbox.extent.y; y < position.y + 0.7; y++)
			{
				for (float z = position.z - bbox.extent.z; z < position.z + bbox.extent.z; z++)
				{
					auto block = AppGlobals::world.getBlock(x, y, z);

					bool isCollidable = AppGlobals::world.blockdb.blockDataFor(block).isCollidable();

					if (isCollidable)
					{
						if (vel.y > 0)
						{
							position.y = y - bbox.extent.y;
							velocity.y = 0;
						}
						else if (vel.y < 0)
						{
							isOnGround = true;
							position.y = y + bbox.extent.y + 1;
							velocity.y = 0;
						}

						if (vel.x > 0)
						{
							position.x = x - bbox.extent.x;
						}
						else if (vel.x < 0)
						{
							position.x = x + bbox.extent.x + 1;
						}

						if (vel.z > 0)
						{
							position.z = z - bbox.extent.z;
						}
						else if (vel.z < 0)
						{
							position.z = z + bbox.extent.z + 1;
						}
					}
				}
			}
		}
	}  
};