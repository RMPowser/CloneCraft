#ifndef RAY_HPP
#define RAY_HPP

#include <cmath>

class Ray {
public:
    Ray(Vec4 position, Vec4 direction) :
        rayStart(position),
        rayEnd(position),
        direction(direction){
        rayStart = position;
        rayEnd = position;
        this->direction = direction;
    }

    void Step(float scale) {
        float yaw = RADIAN * (direction.y + 90);
        float pitch = RADIAN * (direction.x);

        rayEnd.x -= cos(yaw) * scale;
        rayEnd.z -= sin(yaw) * scale;
        rayEnd.y -= tan(pitch) * scale;
        rayEnd.w = 0;
    }

    Vec4 GetEnd() {
        return rayEnd;
    }

    float GetLength() {
        return sqrtf(powf((rayStart.x - rayEnd.x), 2) + powf((rayStart.y - rayEnd.y), 2) + powf((rayStart.z - rayEnd.z), 2));
    }

private:
    Vec4 rayStart;
    Vec4 rayEnd;
    Vec4 direction;
};

#endif // RAY_HPP