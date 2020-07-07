#include "Frustum.h"

enum Planes {
    Near,
    Far,
    Left,
    Right,
    Top,
    Bottom,
};

float Plane::distanceToPoint(const glm::vec3& point) const {
    return glm::dot(point, normal) + distanceToOrigin;
}

void ViewFrustum::update(const glm::mat4& mat) noexcept {
    // left
    planes[Planes::Left].normal.x = mat[0][3] + mat[0][0];
    planes[Planes::Left].normal.y = mat[1][3] + mat[1][0];
    planes[Planes::Left].normal.z = mat[2][3] + mat[2][0];
    planes[Planes::Left].distanceToOrigin = mat[3][3] + mat[3][0];

    // right
    planes[Planes::Right].normal.x = mat[0][3] - mat[0][0];
    planes[Planes::Right].normal.y = mat[1][3] - mat[1][0];
    planes[Planes::Right].normal.z = mat[2][3] - mat[2][0];
    planes[Planes::Right].distanceToOrigin = mat[3][3] - mat[3][0];

    // bottom
    planes[Planes::Bottom].normal.x = mat[0][3] + mat[0][1];
    planes[Planes::Bottom].normal.y = mat[1][3] + mat[1][1];
    planes[Planes::Bottom].normal.z = mat[2][3] + mat[2][1];
    planes[Planes::Bottom].distanceToOrigin = mat[3][3] + mat[3][1];

    // top
    planes[Planes::Top].normal.x = mat[0][3] - mat[0][1];
    planes[Planes::Top].normal.y = mat[1][3] - mat[1][1];
    planes[Planes::Top].normal.z = mat[2][3] - mat[2][1];
    planes[Planes::Top].distanceToOrigin = mat[3][3] - mat[3][1];

    // near
    planes[Planes::Near].normal.x = mat[0][3] + mat[0][2];
    planes[Planes::Near].normal.y = mat[1][3] + mat[1][2];
    planes[Planes::Near].normal.z = mat[2][3] + mat[2][2];
    planes[Planes::Near].distanceToOrigin = mat[3][3] + mat[3][2];

    // far
    planes[Planes::Far].normal.x = mat[0][3] - mat[0][2];
    planes[Planes::Far].normal.y = mat[1][3] - mat[1][2];
    planes[Planes::Far].normal.z = mat[2][3] - mat[2][2];
    planes[Planes::Far].distanceToOrigin = mat[3][3] - mat[3][2];

    for (auto& plane : planes) {
        float length = glm::length(plane.normal);
        plane.normal /= length;
        plane.distanceToOrigin /= length;
    }
}