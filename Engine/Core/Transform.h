#pragma once
#include "Component.h"
#include "../Math/Vector3.h"
#include "../Math/Quaternion.h"
#include "../Math/Matrix4.h"
#include <vector>

namespace Nova {

// -------------------------------------------------------
// Transform – position, rotation, scale + hierarchy.
// Every GameObject owns exactly one Transform.
// -------------------------------------------------------
class Transform : public Component {
public:
    Transform()
        : m_position(Vector3::Zero()),
          m_rotation(Quaternion::Identity()),
          m_scale(Vector3::One()),
          m_parent(nullptr),
          m_localMatrixDirty(true),
          m_worldMatrixDirty(true)
    {}

    std::string GetTypeName() const override { return "Transform"; }

    // --- Local space setters ---
    void SetPosition(const Vector3& pos) { m_position = pos; SetDirty(); }
    void SetRotation(const Quaternion& rot) { m_rotation = rot; SetDirty(); }
    void SetScale(const Vector3& scale)   { m_scale = scale; SetDirty(); }
    void SetEulerAngles(const Vector3& euler) {
        m_rotation = Quaternion::FromEulerDegrees(euler);
        SetDirty();
    }

    // --- Local space getters ---
    const Vector3&     GetPosition()    const { return m_position; }
    const Quaternion&  GetRotation()    const { return m_rotation; }
    const Vector3&     GetScale()       const { return m_scale; }
    Vector3            GetEulerAngles() const { return m_rotation.ToEulerDegrees(); }

    // --- Convenience modifiers ---
    void Translate(const Vector3& delta)  { m_position += delta; SetDirty(); }
    void Rotate(const Quaternion& delta)  { m_rotation = (m_rotation * delta).Normalized(); SetDirty(); }
    void RotateEuler(float px, float py, float pz) {
        Rotate(Quaternion::FromEulerDegrees(px, py, pz));
    }

    // --- World space ---
    Vector3 GetWorldPosition() const { return GetWorldMatrix().TransformPoint(Vector3::Zero()); }

    Vector3 Forward() const { return m_rotation.RotateVector(Vector3::Forward()); }
    Vector3 Up()      const { return m_rotation.RotateVector(Vector3::Up()); }
    Vector3 Right()   const { return m_rotation.RotateVector(Vector3::Right()); }

    // --- Local matrix ---
    const Matrix4& GetLocalMatrix() const {
        if (m_localMatrixDirty) {
            m_localMatrix = Matrix4::FromTRS(m_position, m_rotation, m_scale);
            m_localMatrixDirty = false;
        }
        return m_localMatrix;
    }

    // --- World matrix (cascades through hierarchy) ---
    Matrix4 GetWorldMatrix() const {
        if (m_worldMatrixDirty) {
            if (m_parent) {
                m_worldMatrix = m_parent->GetWorldMatrix() * GetLocalMatrix();
            } else {
                m_worldMatrix = GetLocalMatrix();
            }
            m_worldMatrixDirty = false;
        }
        return m_worldMatrix;
    }

    // --- Hierarchy ---
    void SetParent(Transform* parent) {
        // Remove from old parent
        if (m_parent) {
            auto& siblings = m_parent->m_children;
            siblings.erase(
                std::remove(siblings.begin(), siblings.end(), this),
                siblings.end()
            );
        }
        m_parent = parent;
        if (m_parent) {
            m_parent->m_children.push_back(this);
        }
        SetDirty();
    }

    Transform* GetParent() const { return m_parent; }
    const std::vector<Transform*>& GetChildren() const { return m_children; }
    bool HasChildren() const { return !m_children.empty(); }

    int ChildCount() const { return static_cast<int>(m_children.size()); }
    Transform* GetChild(int index) const {
        if (index < 0 || index >= ChildCount()) return nullptr;
        return m_children[index];
    }

private:
    void SetDirty() {
        m_localMatrixDirty = true;
        MarkWorldDirty();
    }

    void MarkWorldDirty() {
        m_worldMatrixDirty = true;
        for (Transform* child : m_children)
            child->MarkWorldDirty();
    }

    Vector3    m_position;
    Quaternion m_rotation;
    Vector3    m_scale;

    Transform* m_parent;
    std::vector<Transform*> m_children;

    mutable Matrix4 m_localMatrix;
    mutable Matrix4 m_worldMatrix;
    mutable bool    m_localMatrixDirty;
    mutable bool    m_worldMatrixDirty;
};

} // namespace Nova
