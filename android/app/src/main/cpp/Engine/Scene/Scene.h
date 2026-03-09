#pragma once
#include "../Core/GameObject.h"
#include "../Core/Logger.h"
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <functional>

namespace Nova {

// -------------------------------------------------------
// Scene – owns all GameObjects and drives their lifecycle.
// -------------------------------------------------------
class Scene {
public:
    explicit Scene(const std::string& name = "Scene") : m_name(name) {}
    ~Scene() { Destroy(); }

    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& n) { m_name = n; }

    // --- Object management ---

    // Create a new, empty GameObject
    GameObject* CreateGameObject(const std::string& name = "GameObject") {
        auto go = std::make_unique<GameObject>(name);
        GameObject* raw = go.get();
        m_objects.push_back(std::move(go));
        NOVA_LOG_INFO("Scene", "Created GameObject: " + name);
        return raw;
    }

    // Create a child under a parent
    GameObject* CreateChildObject(GameObject* parent, const std::string& name = "Child") {
        GameObject* child = CreateGameObject(name);
        child->GetTransform()->SetParent(parent->GetTransform());
        return child;
    }

    // Destroy an object at end of frame
    void DestroyGameObject(GameObject* obj) {
        if (!obj) return;
        m_pendingDestroy.push_back(obj->GetID());
    }

    // Immediate destroy (use carefully)
    void DestroyGameObjectImmediate(GameObject* obj) {
        if (!obj) return;
        m_objects.erase(
            std::remove_if(m_objects.begin(), m_objects.end(),
                [&](const std::unique_ptr<GameObject>& go) {
                    return go.get() == obj;
                }),
            m_objects.end()
        );
    }

    // Find by name
    GameObject* Find(const std::string& name) const {
        for (auto& go : m_objects)
            if (go->GetName() == name) return go.get();
        return nullptr;
    }

    // Find by tag
    std::vector<GameObject*> FindAllWithTag(const std::string& tag) const {
        std::vector<GameObject*> result;
        for (auto& go : m_objects)
            if (go->GetTag() == tag) result.push_back(go.get());
        return result;
    }

    // Find by ID
    GameObject* FindByID(EntityID id) const {
        for (auto& go : m_objects)
            if (go->GetID() == id) return go.get();
        return nullptr;
    }

    // Get all objects (read-only)
    const std::vector<std::unique_ptr<GameObject>>& GetAllObjects() const {
        return m_objects;
    }

    int ObjectCount() const { return (int)m_objects.size(); }

    // Iterate all objects
    void ForEach(const std::function<void(GameObject*)>& fn) const {
        for (auto& go : m_objects)
            fn(go.get());
    }

    // --- Lifecycle ---
    void Start() {
        for (auto& go : m_objects)
            go->Start();
    }

    void Update(float dt) {
        // Update all active objects
        for (auto& go : m_objects)
            go->Update(dt);

        // Flush pending destroys
        FlushDestroyQueue();
    }

    void FixedUpdate(float fdt) {
        for (auto& go : m_objects)
            go->FixedUpdate(fdt);
    }

    void Render() {
        for (auto& go : m_objects)
            go->Render();
    }

    void Destroy() {
        m_objects.clear();
        m_pendingDestroy.clear();
    }

    // --- Root objects (no parent) ---
    std::vector<GameObject*> GetRootObjects() const {
        std::vector<GameObject*> roots;
        for (auto& go : m_objects)
            if (go->GetTransform()->GetParent() == nullptr)
                roots.push_back(go.get());
        return roots;
    }

private:
    void FlushDestroyQueue() {
        if (m_pendingDestroy.empty()) return;
        for (EntityID id : m_pendingDestroy) {
            m_objects.erase(
                std::remove_if(m_objects.begin(), m_objects.end(),
                    [&](const std::unique_ptr<GameObject>& go) {
                        return go->GetID() == id;
                    }),
                m_objects.end()
            );
        }
        m_pendingDestroy.clear();
    }

    std::string m_name;
    std::vector<std::unique_ptr<GameObject>> m_objects;
    std::vector<EntityID>                    m_pendingDestroy;
};

} // namespace Nova
