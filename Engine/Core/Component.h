#pragma once
#include "Types.h"
#include <string>

namespace Nova {

class GameObject; // forward declaration

// -------------------------------------------------------
// Base class for all components attached to GameObjects.
// -------------------------------------------------------
class Component {
public:
    Component() : m_enabled(true), m_owner(nullptr) {}
    virtual ~Component() = default;

    // Lifecycle callbacks
    virtual void OnAwake()   {} // Called once when component is first added
    virtual void OnStart()   {} // Called before first Update
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnFixedUpdate(float fixedDeltaTime) {}
    virtual void OnDestroy() {}
    virtual void OnEnable()  {}
    virtual void OnDisable() {}

    // Renderer callback – called during render pass
    virtual void OnRender()  {}

    // Inspector data – override to provide component name
    virtual std::string GetTypeName() const { return "Component"; }

    // Enable / disable
    void SetEnabled(bool enabled) {
        if (m_enabled == enabled) return;
        m_enabled = enabled;
        if (m_enabled) OnEnable();
        else           OnDisable();
    }
    bool IsEnabled() const { return m_enabled; }

    // Owner access
    GameObject* GetOwner() const { return m_owner; }
    void SetOwner(GameObject* owner) { m_owner = owner; }

    // Unique type ID per concrete class (set externally by the factory / registry)
    virtual ComponentTypeID GetTypeID() const { return 0; }

protected:
    bool        m_enabled;
    GameObject* m_owner;
};

} // namespace Nova
