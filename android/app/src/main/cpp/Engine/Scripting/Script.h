#pragma once
#include "../Core/Component.h"
#include "../Core/Time.h"
#include "../Core/Logger.h"

namespace Nova {

// -------------------------------------------------------
// Script – base class for user behaviour scripts.
// Users subclass Script and override the lifecycle methods.
// Attach to GameObjects just like any Component.
//
// Example:
//   class SpinScript : public Script {
//   public:
//       void OnUpdate(float dt) override {
//           GetTransform()->RotateEuler(0, 90.0f * dt, 0);
//       }
//   };
// -------------------------------------------------------
class Script : public Component {
public:
    Script() {}
    virtual ~Script() = default;

    std::string GetTypeName() const override { return "Script"; }

    // Convenience helpers available in all scripts

    // Get the transform of the owning GameObject
    Transform* GetTransform() const {
        if (!m_owner) return nullptr;
        return m_owner->GetTransform();
    }

    // Get another component on the same object
    template<typename T>
    T* GetComponent() const {
        if (!m_owner) return nullptr;
        return m_owner->GetComponent<T>();
    }

    // Add a component to the same object
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        if (!m_owner) return nullptr;
        return m_owner->AddComponent<T>(std::forward<Args>(args)...);
    }

    // Engine time
    float DeltaTime()  const { return Time::Get().DeltaTime(); }
    float TotalTime()  const { return Time::Get().TotalTime(); }
    float FrameCount() const { return (float)Time::Get().FrameCount(); }

    // Logging helpers
    void Log(const std::string& msg) const {
        NOVA_LOG_INFO(GetTypeName(), msg);
    }
    void LogWarn(const std::string& msg) const {
        NOVA_LOG_WARN(GetTypeName(), msg);
    }
    void LogError(const std::string& msg) const {
        NOVA_LOG_ERROR(GetTypeName(), msg);
    }
};

} // namespace Nova
