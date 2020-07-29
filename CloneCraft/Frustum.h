#pragma once
#include <array>
#include "glm.h"
#include "AABB.h"

struct Plane {
    float distanceToPoint(Vec3 point);

    float distanceToOrigin;
    Vec3 normal;

    bool operator==(const Plane& other) const {
        return (normal == other.normal && distanceToOrigin == other.distanceToOrigin);
    }

    bool operator!=(const Plane& other) const {
        return !(*this == other);
    }
};

class ViewFrustum {
public:
    void update(glm::mat4& projViewMatrix);
    bool isBoxInFrustum(AABB& box);

    bool operator==(const ViewFrustum& other) const {
        return m_planes == other.m_planes;
    }

    bool operator!=(const ViewFrustum& other) const {
        return !(*this == other);
    }
private:
    std::array<Plane, 6> m_planes;
};
