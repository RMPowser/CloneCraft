#pragma once
#include "Entity.h"
#include <vector>



class Player : public Entity {
    friend class Player;
public:
    Player();

    void update(float dt);
    glm::vec3 m_acceleration;

private:
    
};

