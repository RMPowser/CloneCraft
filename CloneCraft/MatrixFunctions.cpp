#include "MatrixFunctions.h"

#include "Camera.h"
#include "Entity.h"

glm::mat4 MatrixFunctions::makeModelMatrix(const Entity& entity) {
    glm::mat4 matrix;

    matrix = glm::rotate(matrix, glm::radians(entity.rotation.x), { 1, 0, 0 });
    matrix = glm::rotate(matrix, glm::radians(entity.rotation.y), { 0, 1, 0 });
    matrix = glm::rotate(matrix, glm::radians(entity.rotation.z), { 0, 0, 1 });

    matrix = glm::translate(matrix, entity.position);

    return matrix;
}

glm::mat4 MatrixFunctions::makeViewMatrix(const Camera& camera) {
    glm::mat4 matrix(1.f);

    matrix = glm::rotate(matrix, glm::radians(camera.rotation.x), { 1, 0, 0 });
    matrix = glm::rotate(matrix, glm::radians(camera.rotation.y), { 0, 1, 0 });
    matrix = glm::rotate(matrix, glm::radians(camera.rotation.z), { 0, 0, 1 });

    matrix = glm::translate(matrix, -camera.position);

    return matrix;
}

glm::mat4 MatrixFunctions::makeProjectionMatrix(const uint32_t WINDOW_WIDTH, const uint32_t WINDOW_HEIGHT, const uint32_t FOV) {
    float x = (float)WINDOW_WIDTH;
    float y = (float)WINDOW_HEIGHT;
    float fov = (float)FOV;

    return glm::perspective(glm::radians(fov), x / y, 0.1f, 2000.0f);
}
