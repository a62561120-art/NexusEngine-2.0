#pragma once
#include "Types.h"
#include "Component.h"
#include "Transform.h"
#include "../Core/Logger.h"
#include <string>
#include <vector>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <algorithm>

namespace Nova {

// -------------------------------------------------------
// GameObject – the fundamental scene entity.
// Contains a Transform and any number of Components.
// -------------------------------------------------------
class GameObject {
public:
    explicit GameObject(const std::string& name = "GameObject")
        : m_name(name), m_id(UUID::Generate()), m_active(true), m_started(false)
    {
        // Every GameObject gets a Transform automatically
        m_transform = std::make_shared<Transform>();
        m_transform->SetOwner(this);
        m_transform->OnAwake();
    }

    ~GameObject() { OnDestroy(); }

    // --- Identity ---
    const std::string& GetName()   const { return m_name; }
    void               SetName(const std::string& name) { m_name = name; }
    EntityID           GetID()     const { return m_id; }
    std::string        GetTag()    const { return m_tag; }
    void               SetTag(const std::string& tag) { m_tag = tag; }
    int                GetLayer()  const { return m_layer; }
    void               SetLayer(int l)   { m_layer = l; }

    // --- Active state ---
    bool IsActive() const { return m_active; }
    void SetActive(bool active) {
        if (m_active == active) return;
        m_active = active;
        for (auto& [type, comp] : m_components) {
            if (m_active) comp->OnEnable();
            else          comp->OnDisable();
        }
    }

    // --- Transform shortcut ---
    Transform* GetTransform() const { return m_transform.get(); }

    // --- Component system ---
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
        std::type_index tid = std::type_index(typeid(T));
        if (m_components.count(tid)) {
            NOVA_LOG_WARN("GameObject", "Component already exists: " + m_name);
            return static_cast<T*>(m_components[tid].get());
        }
        auto comp = std::make_shared<T>(std::forward<Args>(args)...);
        comp->SetOwner(this);
        comp->OnAwake();
        m_components[tid] = comp;
        m_componentOrder.push_back(tid);
        if (m_started) comp->OnStart(); // late add
        return comp.get();
    }

    template<typename T>
    T* GetComponent() const {
        std::type_index tid = std::type_index(typeid(T));
        auto it = m_components.find(tid);
        if (it != m_components.end())
            return static_cast<T*>(it->second.get());
        return nullptr;
    }

    template<typename T>
    bool HasComponent() const {
        return m_components.count(std::type_index(typeid(T))) > 0;
    }

    template<typename T>
    void RemoveComponent() {
        std::type_index tid = std::type_index(typeid(T));
        auto it = m_components.find(tid);
        if (it != m_components.end()) {
            it->second->OnDestroy();
            m_components.erase(it);
            m_componentOrder.erase(
                std::remove(m_componentOrder.begin(), m_componentOrder.end(), tid),
                m_componentOrder.end());
        }
    }

    // Access all components (for editor, serialization)
    const std::vector<std::type_index>& GetComponentOrder() const { return m_componentOrder; }
    Component* GetComponentByIndex(int i) const {
        if (i < 0 || i >= (int)m_componentOrder.size()) return nullptr;
        return m_components.at(m_componentOrder[i]).get();
    }
    int GetComponentCount() const { return (int)m_componentOrder.size(); }

    // --- Lifecycle ---
    void Start() {
        if (m_started) return;
        m_started = true;
        for (auto& tid : m_componentOrder)
            m_components[tid]->OnStart();
    }

    void Update(float dt) {
        if (!m_active) return;
        for (auto& tid : m_componentOrder) {
            auto& comp = m_components[tid];
            if (comp->IsEnabled())
                comp->OnUpdate(dt);
        }
    }

    void FixedUpdate(float fdt) {
        if (!m_active) return;
        for (auto& tid : m_componentOrder) {
            auto& comp = m_components[tid];
            if (comp->IsEnabled())
                comp->OnFixedUpdate(fdt);
        }
    }

    void Render() {
        if (!m_active) return;
        for (auto& tid : m_componentOrder) {
            auto& comp = m_components[tid];
            if (comp->IsEnabled())
                comp->OnRender();
        }
    }

    void OnDestroy() {
        for (auto& tid : m_componentOrder)
            m_components[tid]->OnDestroy();
        m_components.clear();
        m_componentOrder.clear();
    }

private:
    std::string  m_name;
    std::string  m_tag;
    EntityID     m_id;
    int          m_layer = 0;
    bool         m_active;
    bool         m_started;

    std::shared_ptr<Transform>                                     m_transform;
    std::unordered_map<std::type_index, std::shared_ptr<Component>> m_components;
    std::vector<std::type_index>                                    m_componentOrder;
};

} // namespace Nova
