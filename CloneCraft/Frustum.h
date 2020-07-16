#pragma once
#include <array>
#include "glm.h"

struct Plane {
    float distanceToPoint(glm::vec3& point);

    float distanceToOrigin;
    glm::vec3 normal;
};

class ViewFrustum {
public:
    void update(glm::mat4& projViewMatrix);

private:
    std::array<Plane, 6> m_planes;
};
