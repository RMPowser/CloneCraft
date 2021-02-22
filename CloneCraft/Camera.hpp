#pragma once
#include "Entity.hpp"
#include "Frustum.hpp"

class Camera : public Entity {
	Entity* pEntity;
	ViewFrustum frustum;
	Mat4 projMatrix;
	Mat4 viewMatrix;
	Mat4 projViewMatrix;

public:
	static Mat4 VulkanProjectionMatrix() {
		float fovYRadians = AppGlobals::FOV * RADIAN;
		float aspectRatio = (float)AppGlobals::window.GetClientWidth() / (float)AppGlobals::window.GetClientHeight();
		float zNear = 0.1;
		float zFar = 2000;

		static GW::MATH::GMATRIXF r;
		GW::MATH::GMatrix::ProjectionVulkanRHF(fovYRadians, aspectRatio, zNear, zFar, r);
		return Mat4(r.row1.x, r.row1.y, r.row1.z, r.row1.w,
					r.row2.x, r.row2.y, r.row2.z, r.row2.w, 
					r.row3.x, r.row3.y, r.row3.z, r.row3.w, 
					r.row4.x, r.row4.y, r.row4.z, r.row4.w);
	}

	static Mat4 MakeObjMatrix(Entity& entity) {
		Mat4 m = Mat4::IdentityMatrix();
		m.LocalRotateX(entity.rotation.x * RADIAN);
		m.LocalRotateY(entity.rotation.y * RADIAN);
		m.LocalRotateZ(entity.rotation.z * RADIAN);
		m.Translate(entity.position);
		return m;
	}

	static Mat4 MakeViewMatrix(Entity& entity) {
		Mat4 m = Mat4::IdentityMatrix();
		m.LocalRotateX(entity.rotation.x * RADIAN);
		m.LocalRotateY(entity.rotation.y * RADIAN);
		m.LocalRotateZ(entity.rotation.z * RADIAN);
		m.Translate(Vec4(-entity.position.x, -entity.position.y, -entity.position.z, 0.f));
		return m;
	}

	Camera() {
		projMatrix = VulkanProjectionMatrix();
	}

	void Update() {
		assert(pEntity);
		position = pEntity->position;
		position.y += pEntity->height - 0.05f;
		rotation.y = pEntity->rotation.y;

		viewMatrix = MakeViewMatrix(*this);
		projViewMatrix = projMatrix * viewMatrix;
		frustum.update(projViewMatrix);
	}

	void HookEntity(Entity& entity) {
		pEntity = &entity;
	}

	void RecreateProjectionMatrix() {
		projMatrix = VulkanProjectionMatrix();
	}

	Mat4& getViewMatrix()			{ return viewMatrix; }
	Mat4& getProjMatrix()			{ return projMatrix; }
	Mat4& getProjectionViewMatrix()	{ return projViewMatrix; }
	ViewFrustum& getFrustum()		{ return frustum; }

	bool operator==(const Camera& other) const {
		return (pEntity == other.pEntity &&
				frustum == other.frustum &&
				projMatrix == other.projMatrix &&
				viewMatrix == other.viewMatrix &&
				projViewMatrix == other.projViewMatrix);
	}

	bool operator!=(const Camera& other) const {
		return !(*this == other);
	}
};