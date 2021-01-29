#pragma once
#include <cmath>

class Ray {
public:
    Ray(GW::MATH::GVECTORF& position, GW::MATH::GVECTORF& direction) :
        rayStart(position),
        rayEnd(position),
        direction(direction){
    }

    void step(float scale) {
        float yaw = AppGlobals::RADIAN * (direction.y + 90);
        float pitch = AppGlobals::RADIAN * (direction.x);

        rayEnd.x -= cos(yaw) * scale;
        rayEnd.z -= sin(yaw) * scale;
        rayEnd.y -= tan(pitch) * scale;
        rayEnd.w = 0;
    }

    GW::MATH::GVECTORF& getEnd() {
        return rayEnd;
    }

    float getLength() {
        return sqrtf(powf((rayStart.x - rayEnd.x), 2) + powf((rayStart.y - rayEnd.y), 2) + powf((rayStart.z - rayEnd.z), 2));
    }

private:
    GW::MATH::GVECTORF rayStart;
    GW::MATH::GVECTORF rayEnd;
    GW::MATH::GVECTORF direction;
};