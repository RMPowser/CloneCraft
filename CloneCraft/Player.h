#pragma once
#include "Entity.h"
#include "World.h"
#include "AABB.h"
#include <vector>



class Player : public Entity {
public:
    Player();

    void update(float dt);
    void collide(World& world, const glm::vec3& vel);
    glm::vec3 acceleration;

    World* world;
};

