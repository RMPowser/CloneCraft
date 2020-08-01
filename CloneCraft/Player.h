#pragma once
#include "Entity.h"
#include "World.h"
#include "Vertex.h"
#include "Controller.h"
#include <vector>



class Player : public Entity {
public:
    Player();

    void update(float dt, Controller controller);
    void collide(World& world, const glm::vec3& vel);
    Vec3 acceleration;

    World* world;
    bool isOnGround;
    bool isFlying;
};

