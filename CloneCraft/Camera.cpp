#include "Camera.h"
#include "AppConfig.h"
#include "MatrixFunctions.h"

Camera::Camera(AppConfig& config)
    : m_config(config) {
    m_projectionMatrix = makeProjectionMatrix(config);
    position = { 0, 0, -3.5 };
}

void Camera::update(){
    position = { m_pEntity->position.x, m_pEntity->position.y + 0.6f, m_pEntity->position.z };
    rotation = m_pEntity->rotation;

    m_viewMatrix = makeViewMatrix(*this);
    m_projViewMatrx = m_projectionMatrix * m_viewMatrix;
    m_frustum.update(m_projViewMatrx);
}

void Camera::hookEntity(Entity& entity) {
    m_pEntity = &entity;
}

glm::mat4& Camera::getViewMatrix() {
    return m_viewMatrix;
}

glm::mat4& Camera::getProjMatrix() {
    return m_projectionMatrix;
}

void Camera::recreateProjectionMatrix(AppConfig& config) {
    m_projectionMatrix = makeProjectionMatrix(config);
}

glm::mat4& Camera::getProjectionViewMatrix() {
    return m_projViewMatrx;
}

ViewFrustum& Camera::getFrustum() {
    return m_frustum;
}
