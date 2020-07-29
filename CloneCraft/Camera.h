#pragma once
#include "AppConfig.h"
#include "Entity.h"
#include "Frustum.h"
#include "Vertex.h"
#include "glm.h"

class Camera : public Entity {
public:
    Camera(AppConfig& config);

    void update();
    void hookEntity(Entity& entity);

    glm::mat4& getViewMatrix();
    glm::mat4& getProjMatrix();
    void recreateProjectionMatrix(AppConfig& config);
    glm::mat4& getProjectionViewMatrix();

    ViewFrustum& getFrustum();

private:
    Entity* m_pEntity;

    ViewFrustum m_frustum;

    glm::mat4 m_projectionMatrix;
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projViewMatrx;

    AppConfig m_config;
};