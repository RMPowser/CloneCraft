#include <array>

#include "glm.h"


struct Plane {
    float distanceToPoint(const glm::vec3& point) const;

    float distanceToOrigin;
    glm::vec3 normal;
};

class ViewFrustum {
public:
    void update(const glm::mat4& projViewMatrix) noexcept;

private:
    std::array<Plane, 6> planes;
};