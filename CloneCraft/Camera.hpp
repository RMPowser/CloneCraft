#pragma once
#include "Entity.hpp"
#include "Frustum.hpp"


static GW::MATH::GMATRIXF makeObjMatrix(Entity& entity)
{
    GW::MATH::GMATRIXF matrix{ GW::MATH::GIdentityMatrixF };

    GW::MATH::GMatrix::RotateXGlobalF(matrix, entity.rotation.x * AppGlobals::RADIAN, matrix);
    GW::MATH::GMatrix::RotateYGlobalF(matrix, entity.rotation.y * AppGlobals::RADIAN, matrix);
    GW::MATH::GMatrix::RotateZGlobalF(matrix, entity.rotation.z * AppGlobals::RADIAN, matrix);

    GW::MATH::GMatrix::TranslateLocalF(matrix, entity.position, matrix);

    return matrix;
}

static GW::MATH::GMATRIXF makeViewMatrix(Entity& entity)
{
    GW::MATH::GMATRIXF matrix(GW::MATH::GIdentityMatrixF);

    GW::MATH::GMatrix::RotateXGlobalF(matrix, entity.rotation.x * AppGlobals::RADIAN, matrix);
    GW::MATH::GMatrix::RotateYGlobalF(matrix, entity.rotation.y * AppGlobals::RADIAN, matrix);
    GW::MATH::GMatrix::RotateZGlobalF(matrix, entity.rotation.z * AppGlobals::RADIAN, matrix);

    GW::MATH::GMatrix::TranslateLocalF(matrix, GW::MATH::GVECTORF{ -entity.position.x, -entity.position.y, -entity.position.z, 0 }, matrix);

    return matrix;
}

static GW::MATH::GMATRIXF makeProjectionMatrix()
{
    float x = (float)AppGlobals::windowX;
    float y = (float)AppGlobals::windowY;
    float aspect = x / y;
    float fov = AppGlobals::FOV * AppGlobals::RADIAN;
    float zNear = 0.1;
    float zFar = 2000;

    GW::MATH::GMATRIXF Result;
    GW::MATH::GMatrix::ProjectionVulkanRHF(fov, aspect, zNear, zFar, Result);

    return Result;
}

class Camera : public Entity
{
public:
    Camera()
    {
        m_projectionMatrix = makeProjectionMatrix();
        position = { 0, 0, -3.5, 0 };
    }

    void update()
    {
        position = m_pEntity->position;
        position.y += 0.6f;
        rotation = m_pEntity->rotation;

        m_viewMatrix = makeViewMatrix(*this);
        GW::MATH::GMatrix::MultiplyMatrixF(m_projectionMatrix, m_viewMatrix, m_projViewMatrx);
        m_frustum.update(m_projViewMatrx);
    }

    void hookEntity(Entity& entity)
    {
        m_pEntity = &entity;
    }

    GW::MATH::GMATRIXF& getViewMatrix() { return m_viewMatrix; }
    GW::MATH::GMATRIXF& getProjMatrix() { return m_projectionMatrix; }
    void recreateProjectionMatrix() { m_projectionMatrix = makeProjectionMatrix(); }
    GW::MATH::GMATRIXF& getProjectionViewMatrix() { return m_projViewMatrx; }

    ViewFrustum& getFrustum() { return m_frustum; }

    bool operator==(const Camera& other) const
    {
        return (m_pEntity == other.m_pEntity &&
                m_frustum == other.m_frustum &&
                m_projectionMatrix.row1.x == other.m_projectionMatrix.row1.x &&
                m_projectionMatrix.row1.y == other.m_projectionMatrix.row1.y &&
                m_projectionMatrix.row1.z == other.m_projectionMatrix.row1.z &&
                m_projectionMatrix.row1.w == other.m_projectionMatrix.row1.w &&
                m_projectionMatrix.row2.x == other.m_projectionMatrix.row2.x &&
                m_projectionMatrix.row2.y == other.m_projectionMatrix.row2.y &&
                m_projectionMatrix.row2.z == other.m_projectionMatrix.row2.z &&
                m_projectionMatrix.row2.w == other.m_projectionMatrix.row2.w &&
                m_projectionMatrix.row3.x == other.m_projectionMatrix.row3.x &&
                m_projectionMatrix.row3.y == other.m_projectionMatrix.row3.y &&
                m_projectionMatrix.row3.z == other.m_projectionMatrix.row3.z &&
                m_projectionMatrix.row3.w == other.m_projectionMatrix.row3.w &&
                m_projectionMatrix.row4.x == other.m_projectionMatrix.row4.x &&
                m_projectionMatrix.row4.y == other.m_projectionMatrix.row4.y &&
                m_projectionMatrix.row4.z == other.m_projectionMatrix.row4.z &&
                m_projectionMatrix.row4.w == other.m_projectionMatrix.row4.w &&
                
                m_viewMatrix.row1.x == other.m_viewMatrix.row1.x &&
                m_viewMatrix.row1.y == other.m_viewMatrix.row1.y &&
                m_viewMatrix.row1.z == other.m_viewMatrix.row1.z &&
                m_viewMatrix.row1.w == other.m_viewMatrix.row1.w &&
                m_viewMatrix.row2.x == other.m_viewMatrix.row2.x &&
                m_viewMatrix.row2.y == other.m_viewMatrix.row2.y &&
                m_viewMatrix.row2.z == other.m_viewMatrix.row2.z &&
                m_viewMatrix.row2.w == other.m_viewMatrix.row2.w &&
                m_viewMatrix.row3.x == other.m_viewMatrix.row3.x &&
                m_viewMatrix.row3.y == other.m_viewMatrix.row3.y &&
                m_viewMatrix.row3.z == other.m_viewMatrix.row3.z &&
                m_viewMatrix.row3.w == other.m_viewMatrix.row3.w &&
                m_viewMatrix.row4.x == other.m_viewMatrix.row4.x &&
                m_viewMatrix.row4.y == other.m_viewMatrix.row4.y &&
                m_viewMatrix.row4.z == other.m_viewMatrix.row4.z &&
                m_viewMatrix.row4.w == other.m_viewMatrix.row4.w &&
                
                m_projViewMatrx.row1.x == other.m_projViewMatrx.row1.x &&
                m_projViewMatrx.row1.y == other.m_projViewMatrx.row1.y &&
                m_projViewMatrx.row1.z == other.m_projViewMatrx.row1.z &&
                m_projViewMatrx.row1.w == other.m_projViewMatrx.row1.w &&
                m_projViewMatrx.row2.x == other.m_projViewMatrx.row2.x &&
                m_projViewMatrx.row2.y == other.m_projViewMatrx.row2.y &&
                m_projViewMatrx.row2.z == other.m_projViewMatrx.row2.z &&
                m_projViewMatrx.row2.w == other.m_projViewMatrx.row2.w &&
                m_projViewMatrx.row3.x == other.m_projViewMatrx.row3.x &&
                m_projViewMatrx.row3.y == other.m_projViewMatrx.row3.y &&
                m_projViewMatrx.row3.z == other.m_projViewMatrx.row3.z &&
                m_projViewMatrx.row3.w == other.m_projViewMatrx.row3.w &&
                m_projViewMatrx.row4.x == other.m_projViewMatrx.row4.x &&
                m_projViewMatrx.row4.y == other.m_projViewMatrx.row4.y &&
                m_projViewMatrx.row4.z == other.m_projViewMatrx.row4.z &&
                m_projViewMatrx.row4.w == other.m_projViewMatrx.row4.w);
    }

    bool operator!=(const Camera& other) const
    {
        return !(*this == other);
    }

private:
    Entity* m_pEntity;

    ViewFrustum m_frustum;

    GW::MATH::GMATRIXF m_projectionMatrix;
    GW::MATH::GMATRIXF m_viewMatrix;
    GW::MATH::GMATRIXF m_projViewMatrx;
};