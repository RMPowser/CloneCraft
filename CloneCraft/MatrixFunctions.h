#pragma once
#include "glm.h"

class Camera;
class Entity;

class MatrixFunctions {
public:
    static glm::mat4 makeModelMatrix(const Entity& entity);
    static glm::mat4 makeViewMatrix(const Camera& camera);
    static glm::mat4 makeProjectionMatrix(const uint32_t WINDOW_WIDTH, const uint32_t WINDOW_HEIGHT, const uint32_t FOV);

private:
    MatrixFunctions() {};
};