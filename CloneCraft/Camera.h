#include "Entity.h"
#include "MatrixFunctions.h"
#include "Frustum.h"

class Camera : public Entity {
public:
	Camera() {}
	~Camera() {}

	void update() {
		position = { playerEntity->position.x, playerEntity->position.y + 0.6f, playerEntity->position.z };
		rotation = playerEntity->rotation;

		viewMatrix = MatrixFunctions::makeViewMatrix(*this);
		projViewMatrx = projectionMatrix * viewMatrix;
		frustum.update(projViewMatrx);
	}

	void attachPlayerEntity(const Entity& player) noexcept {
		playerEntity = &player;
	}
	
	const glm::mat4& getViewMatrix() const noexcept {
		return viewMatrix;
	}

	const glm::mat4& getProjMatrix() const noexcept {
		return projectionMatrix;
	}

	const glm::mat4& getProjectionViewMatrix() const noexcept {
		return projViewMatrx;
	}

	const ViewFrustum& getFrustum() const noexcept {
		return frustum;
	}

	

private:
	const Entity* playerEntity = nullptr;

	ViewFrustum frustum;

	glm::mat4 projectionMatrix;
	glm::mat4 viewMatrix;
	glm::mat4 projViewMatrx;
};
