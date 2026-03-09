#pragma once
#include "Scene.h"
#include "../Core/Logger.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace Nova {

// -------------------------------------------------------
// SceneManager – manages loading / switching scenes.
// -------------------------------------------------------
class SceneManager {
public:
    static SceneManager& Get() {
        static SceneManager instance;
        return instance;
    }

    // Create and register a scene
    Scene* CreateScene(const std::string& name) {
        auto scene = std::make_unique<Scene>(name);
        Scene* raw = scene.get();
        m_scenes.push_back(std::move(scene));
        NOVA_LOG_INFO("SceneManager", "Created scene: " + name);
        if (!m_activeScene) m_activeScene = raw;
        return raw;
    }

    // Load (switch to) a scene by name
    bool LoadScene(const std::string& name) {
        for (auto& s : m_scenes) {
            if (s->GetName() == name) {
                SwitchTo(s.get());
                return true;
            }
        }
        NOVA_LOG_ERROR("SceneManager", "Scene not found: " + name);
        return false;
    }

    // Get active scene
    Scene* GetActiveScene() const { return m_activeScene; }

    // Set which scene is active (without lifecycle calls)
    void SetActiveScene(Scene* scene) { m_activeScene = scene; }

    // Update / render the active scene
    void Update(float dt) {
        if (m_nextScene) {
            if (m_activeScene) m_activeScene->Destroy();
            m_activeScene = m_nextScene;
            m_activeScene->Start();
            m_nextScene = nullptr;
        }
        if (m_activeScene) m_activeScene->Update(dt);
    }

    void FixedUpdate(float fdt) {
        if (m_activeScene) m_activeScene->FixedUpdate(fdt);
    }

    void Render() {
        if (m_activeScene) m_activeScene->Render();
    }

    // Get all registered scenes
    int SceneCount() const { return (int)m_scenes.size(); }
    Scene* GetSceneAt(int i) const {
        if (i < 0 || i >= (int)m_scenes.size()) return nullptr;
        return m_scenes[i].get();
    }

private:
    SceneManager() : m_activeScene(nullptr), m_nextScene(nullptr) {}

    void SwitchTo(Scene* scene) {
        m_nextScene = scene; // deferred to next Update
        NOVA_LOG_INFO("SceneManager", "Queued scene switch to: " + scene->GetName());
    }

    std::vector<std::unique_ptr<Scene>> m_scenes;
    Scene* m_activeScene;
    Scene* m_nextScene;
};

} // namespace Nova
