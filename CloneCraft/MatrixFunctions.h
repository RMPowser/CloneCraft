#pragma once
#include "glm.h"
#include "Entity.h"
#include "Camera.h"

static glm::mat4 makeObjMatrix(Entity& entity) {
    glm::mat4 matrix;

    matrix = glm::rotate(matrix, glm::radians(entity.rotation.x), { 1, 0, 0 });
    matrix = glm::rotate(matrix, glm::radians(entity.rotation.y), { 0, 1, 0 });
    matrix = glm::rotate(matrix, glm::radians(entity.rotation.z), { 0, 0, 1 });

    matrix = glm::translate(matrix, entity.position);

    return matrix;
}

static glm::mat4 makeViewMatrix(Camera& camera) {
    glm::mat4 matrix(1.0f);

    matrix = glm::rotate(matrix, glm::radians(camera.rotation.x), { 1, 0, 0 });
    matrix = glm::rotate(matrix, glm::radians(camera.rotation.y), { 0, 1, 0 });
    matrix = glm::rotate(matrix, glm::radians(camera.rotation.z), { 0, 0, 1 });

    matrix = glm::translate(matrix, -camera.position);

    return matrix;
}

static glm::mat4 makeProjectionMatrix(AppConfig config) {
    float x = (float)config.windowX;
    float y = (float)config.windowY;
    float fov = (float)75;

    return glm::perspective(glm::radians(fov), x / y, 0.1f, 2000.0f);
}
