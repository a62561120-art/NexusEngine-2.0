#include "Light.h"
#include "../Core/GameObject.h"
#include "../Core/Transform.h"

namespace Nova {

Vector3 Light::GetDirection() const {
    if (!m_owner) return {0, -1, 0};
    return m_owner->GetTransform()->Forward();
}

Vector3 Light::GetPosition() const {
    if (!m_owner) return Vector3::Zero();
    return m_owner->GetTransform()->GetWorldPosition();
}

} // namespace Nova
