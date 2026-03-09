#pragma once
#include "../Core/Component.h"
#include "../Math/Matrix4.h"
#include "../Math/Vector3.h"
#include <cmath>

namespace Nova {

enum class CameraProjection { Perspective, Orthographic };

struct ClearColorData { float r, g, b, a; };

// -------------------------------------------------------
// Camera – view and projection matrices.
// Attach to a GameObject; its Transform drives position/
// orientation automatically.
// -------------------------------------------------------
class Camera : public Component {
public:
    Camera()
        : m_projection(CameraProjection::Perspective),
          m_fov(60.0f), m_aspect(16.0f/9.0f),
          m_near(0.1f), m_far(1000.0f),
          m_orthoSize(5.0f),
          m_depth(0),
          m_dirtyProj(true)
    {
        m_clearColor = {0.1f, 0.12f, 0.18f, 1.0f};
    }

    std::string GetTypeName() const override { return "Camera"; }

    // --- Settings ---
    void SetFOV(float fov)          { m_fov = fov;     m_dirtyProj = true; }
    void SetAspect(float aspect)    { m_aspect = aspect; m_dirtyProj = true; }
    void SetNear(float n)           { m_near = n;       m_dirtyProj = true; }
    void SetFar(float f)            { m_far = f;        m_dirtyProj = true; }
    void SetOrthoSize(float s)      { m_orthoSize = s;  m_dirtyProj = true; }
    void SetProjection(CameraProjection p) { m_projection = p; m_dirtyProj = true; }
    void SetClearColor(float r, float g, float b, float a = 1.0f) {
        m_clearColor = {r, g, b, a};
    }
    void SetDepth(int d) { m_depth = d; }

    float GetFOV()      const { return m_fov; }
    float GetAspect()   const { return m_aspect; }
    float GetNear()     const { return m_near; }
    float GetFar()      const { return m_far; }
    int   GetDepth()    const { return m_depth; }
    CameraProjection GetProjectionType() const { return m_projection; }
    ClearColorData GetClearColor() const { return m_clearColor; }

    // --- Matrices ---
    Matrix4 GetProjectionMatrix() const {
        if (m_dirtyProj) {
            if (m_projection == CameraProjection::Perspective) {
                m_projMatrix = Matrix4::Perspective(m_fov, m_aspect, m_near, m_far);
            } else {
                float h = m_orthoSize, w = h * m_aspect;
                m_projMatrix = Matrix4::Orthographic(-w, w, -h, h, m_near, m_far);
            }
            m_dirtyProj = false;
        }
        return m_projMatrix;
    }

    // View matrix derived from owner's Transform
    Matrix4 GetViewMatrix() const;

    // World-space position of the camera
    Vector3 GetPosition() const;

    // Build a view matrix from explicit params (useful for shadow cams etc.)
    static Matrix4 BuildViewMatrix(const Vector3& eye, const Vector3& center, const Vector3& up) {
        return Matrix4::LookAt(eye, center, up);
    }

    // --- Utility ---
    // Convert screen UV [0,1] to world-space ray direction
    Vector3 ScreenPointToRay(float uvX, float uvY) const;

private:
    CameraProjection m_projection;
    float m_fov, m_aspect, m_near, m_far, m_orthoSize;
    ClearColorData m_clearColor;
    int   m_depth;

    mutable Matrix4 m_projMatrix;
    mutable bool    m_dirtyProj;
};

} // namespace Nova
