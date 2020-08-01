#pragma once
#include "Vertex.h"
#include "Radian.h"
#include <cmath>

class Ray {
public:
    Ray(Vec3& position, Vec3& direction) :
        rayStart(position),
        rayEnd(position),
        direction(direction){
    }

    void step(float scale) {
        float yaw = RADIAN * (direction.y + 90);
        float pitch = RADIAN * (direction.x);

        auto& p = rayEnd;

        p.x -= cos(yaw) * scale;
        p.z -= sin(yaw) * scale;
        p.y -= tan(pitch) * scale;
    }

    Vec3& getEnd() {
        return rayEnd;
    }

    float getLength() {
        return Vec3::distance(rayStart, rayEnd);
    }

private:
    Vec3 rayStart;
    Vec3 rayEnd;
    Vec3 direction;
};