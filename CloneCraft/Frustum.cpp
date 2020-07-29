#pragma once
#include "Frustum.h"

enum Planes {
	Near,
	Far,
	Left,
	Right,
	Top,
	Bottom,
};

float Plane::distanceToPoint(Vec3 point) {
	return Vec3::dot(point, normal) + distanceToOrigin;
}

void ViewFrustum::update(glm::mat4& mat) {
	// left
	m_planes[Planes::Left].distanceToOrigin = mat[3][3] + mat[3][0];
	m_planes[Planes::Left].normal = {   mat[0][3] - mat[0][0],
										mat[1][3] - mat[1][0], 
										mat[2][3] - mat[2][0] };
	
	// right
	m_planes[Planes::Right].distanceToOrigin = mat[3][3] - mat[3][0];
	m_planes[Planes::Right].normal = {  mat[0][3] - mat[0][0],
										mat[1][3] - mat[1][0],
										mat[2][3] - mat[2][0] };
	
	// bottom
	m_planes[Planes::Bottom].distanceToOrigin = mat[3][3] + mat[3][1];
	m_planes[Planes::Bottom].normal = { mat[0][3] + mat[0][1], 
										mat[1][3] + mat[1][1], 
										mat[2][3] - mat[2][1] };
	
	// top
	m_planes[Planes::Top].distanceToOrigin = mat[3][3] - mat[3][1];
	m_planes[Planes::Top].normal = {	mat[0][3] + mat[0][1],
										mat[1][3] + mat[1][1], 
										mat[2][3] - mat[2][1] };

	// near
	m_planes[Planes::Near].distanceToOrigin = mat[3][3] + mat[3][2];
	m_planes[Planes::Near].normal = {	mat[0][3] + mat[0][2],
										mat[1][3] + mat[1][2],
										mat[2][3] + mat[2][2] };

	// far
	m_planes[Planes::Far].distanceToOrigin = mat[3][3] - mat[3][2];
	m_planes[Planes::Far].normal = {	mat[0][3] - mat[0][2],
										mat[1][3] - mat[1][2],
										mat[2][3] - mat[2][2] };

	for (auto& plane : m_planes) {
		float length = Vec3::length(plane.normal);
		plane.normal /= length;
		plane.distanceToOrigin /= length;
	}
}

bool ViewFrustum::isBoxInFrustum(AABB& box) {
	bool result = true;
	for (auto& plane : m_planes) {
		if (plane.distanceToPoint(box.getVPositive(plane.normal)) < 0) {
			return false;
		} else if (plane.distanceToPoint(box.getVNegative(plane.normal)) < 0) {
			result = true;
		}
	}
	return result;
}


