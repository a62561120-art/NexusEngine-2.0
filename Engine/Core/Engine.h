#pragma once
#include "Time.h"
#include "Logger.h"
#include "../Scene/SceneManager.h"
#include <string>
#include <functional>

namespace Nova {

struct EngineConfig {
    std::string appName  = "NovaEngine App";
    int         width    = 800;
    int         height   = 600;
    bool        vsync    = true;
    float       fixedDT  = 1.0f / 60.0f;
    LogLevel    logLevel = LogLevel::Info;
};

class Engine {
public:
    static Engine& Get() {
        static Engine instance;
        return instance;
    }

    bool Init(const EngineConfig& cfg = EngineConfig()) {
        m_config = cfg;
        Logger::Get().SetLevel(cfg.logLevel);
        NOVA_LOG_INFO("Engine", "Initialising " + cfg.appName);
        Time::Get().Init();
        Time::Get().SetFixedDeltaTime(cfg.fixedDT);
        SceneManager::Get().CreateScene("Main");
        NOVA_LOG_INFO("Engine", "Engine ready.");
        m_initialised = true;
        return true;
    }

    void SetOnStart(std::function<void()> fn)            { m_onStart = fn; }
    void SetOnUpdate(std::function<void(float)> fn)      { m_onUpdate = fn; }
    void SetOnFixedUpdate(std::function<void(float)> fn) { m_onFixed = fn; }
    void SetOnRender(std::function<void()> fn)           { m_onRender = fn; }
    void SetOnShutdown(std::function<void()> fn)         { m_onShutdown = fn; }

    Scene* GetScene() const { return SceneManager::Get().GetActiveScene(); }

    void StartSystems() {
        if (!m_initialised) return;
        if (m_onStart) m_onStart();
        if (GetScene()) GetScene()->Start();
        NOVA_LOG_INFO("Engine", "Systems started.");
    }

    void StepFrame() {
        Time::Get().Tick();
        float dt = Time::Get().DeltaTime();
        m_fixedAccum += dt;
        float fdt = Time::Get().FixedDeltaTime();
        while (m_fixedAccum >= fdt) {
            if (m_onFixed) m_onFixed(fdt);
            SceneManager::Get().FixedUpdate(fdt);
            m_fixedAccum -= fdt;
        }
        if (m_onUpdate) m_onUpdate(dt);
        SceneManager::Get().Update(dt);
        if (m_onRender) m_onRender();
        SceneManager::Get().Render();
    }

    void Shutdown() {
        NOVA_LOG_INFO("Engine", "Shutting down.");
        if (m_onShutdown) m_onShutdown();
        m_running = false;
    }

    bool IsRunning() const { return m_running; }
    void RequestQuit()     { m_running = false; }
    const EngineConfig& GetConfig() const { return m_config; }

private:
    Engine() : m_initialised(false), m_running(true), m_fixedAccum(0.0f) {}
    EngineConfig m_config;
    bool         m_initialised;
    bool         m_running;
    float        m_fixedAccum;
    std::function<void()>      m_onStart;
    std::function<void(float)> m_onUpdate;
    std::function<void(float)> m_onFixed;
    std::function<void()>      m_onRender;
    std::function<void()>      m_onShutdown;
};

} // namespace Nova
