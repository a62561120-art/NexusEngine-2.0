#include "Camera.h"
#include "../Core/GameObject.h"
#include "../Core/Transform.h"
#include <cmath>

namespace Nova {

Matrix4 Camera::GetViewMatrix() const {
    if (!m_owner) return Matrix4::Identity();
    Transform* t = m_owner->GetTransform();
    Vector3 pos     = t->GetWorldPosition();
    Vector3 forward = t->Forward();
    Vector3 up      = t->Up();
    return Matrix4::LookAt(pos, pos + forward, up);
}

Vector3 Camera::GetPosition() const {
    if (!m_owner) return Vector3::Zero();
    return m_owner->GetTransform()->GetWorldPosition();
}

Vector3 Camera::ScreenPointToRay(float uvX, float uvY) const {
    // Convert UV to NDC [-1,1]
    float ndcX =  2.0f * uvX - 1.0f;
    float ndcY = -2.0f * uvY + 1.0f;

    // Unproject through inverse projection and view
    const float DEG2RAD = 3.14159265f / 180.0f;
    float tanHalfFov = std::tan(m_fov * DEG2RAD * 0.5f);
    Vector3 rayView(ndcX * m_aspect * tanHalfFov,
                    ndcY * tanHalfFov,
                    -1.0f);

    // Transform to world space
    if (!m_owner) return rayView.Normalized();
    Transform* t = m_owner->GetTransform();
    Vector3 right   = t->Right();
    Vector3 up      = t->Up();
    Vector3 forward = t->Forward();
    Vector3 world = right * rayView.x + up * rayView.y + forward * (-rayView.z);
    return world.Normalized();
}

} // namespace Nova
