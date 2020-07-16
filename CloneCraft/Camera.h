
#pragma once
#include "AppConfig.h"
#include "Entity.h"
#include "MatrixFunctions.h"
#include "Frustum.h"
#include "glm.h"

class Camera : public Entity {
public:
	Camera() {}
	~Camera() {}
    Camera(AppConfig& config);

	void update() {
		position = { playerEntity->position.x, playerEntity->position.y + 0.6f, playerEntity->position.z };
		rotation = playerEntity->rotation;
    void update();
    void hookEntity(Entity& entity);

		viewMatrix = MatrixFunctions::makeViewMatrix(*this);
		projViewMatrx = projectionMatrix * viewMatrix;
		frustum.update(projViewMatrx);
	}
    glm::mat4& getViewMatrix();
    glm::mat4& getProjMatrix();
    void recreateProjectionMatrix(AppConfig& config);
    glm::mat4& getProjectionViewMatrix();

	void attachPlayerEntity(const Entity& player) noexcept {
		playerEntity = &player;
	}
	
	const glm::mat4& getViewMatrix() const noexcept {
		return viewMatrix;
	}
    ViewFrustum& getFrustum();

	const glm::mat4& getProjMatrix() const noexcept {
		return projectionMatrix;
	}
private:
    Entity* m_pEntity;

	const glm::mat4& getProjectionViewMatrix() const noexcept {
		return projViewMatrx;
	}
    ViewFrustum m_frustum;

	const ViewFrustum& getFrustum() const noexcept {
		return frustum;
	}
    glm::mat4 m_projectionMatrix;
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projViewMatrx;

	

private:
	const Entity* playerEntity = nullptr;

	ViewFrustum frustum;

	glm::mat4 projectionMatrix;
	glm::mat4 viewMatrix;
	glm::mat4 projViewMatrx;
};
    AppConfig m_config;
};