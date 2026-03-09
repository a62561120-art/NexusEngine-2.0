#pragma once
#include "../Nova.h"
#include <cmath>

namespace Nova {

// -------------------------------------------------------
// RotateScript – spins the object around its Y axis
// -------------------------------------------------------
class RotateScript : public Script {
public:
    float speed = 45.0f; // degrees per second

    std::string GetTypeName() const override { return "RotateScript"; }

    void OnUpdate(float dt) override {
        GetTransform()->RotateEuler(0, speed * dt, 0);
    }
};

// -------------------------------------------------------
// BobScript – oscillates the object up and down
// -------------------------------------------------------
class BobScript : public Script {
public:
    float amplitude = 0.5f;
    float frequency = 1.0f;

    std::string GetTypeName() const override { return "BobScript"; }

    void OnStart() override {
        m_baseY = GetTransform()->GetPosition().y;
    }

    void OnUpdate(float dt) override {
        float t = TotalTime();
        Vector3 pos = GetTransform()->GetPosition();
        pos.y = m_baseY + std::sin(t * frequency * 3.14159f * 2.0f) * amplitude;
        GetTransform()->SetPosition(pos);
    }

private:
    float m_baseY = 0.0f;
};

// -------------------------------------------------------
// OrbitScript – orbits around a target point
// -------------------------------------------------------
class OrbitScript : public Script {
public:
    Vector3 center  = Vector3::Zero();
    float   radius  = 3.0f;
    float   speed   = 60.0f; // degrees per second
    float   height  = 0.0f;

    std::string GetTypeName() const override { return "OrbitScript"; }

    void OnUpdate(float dt) override {
        m_angle += speed * dt;
        const float DEG2RAD = 3.14159265f / 180.0f;
        float rad = m_angle * DEG2RAD;
        Vector3 pos = {
            center.x + radius * std::cos(rad),
            center.y + height,
            center.z + radius * std::sin(rad)
        };
        GetTransform()->SetPosition(pos);
    }

private:
    float m_angle = 0.0f;
};

// -------------------------------------------------------
// FPSLogScript – logs FPS every N seconds
// -------------------------------------------------------
class FPSLogScript : public Script {
public:
    float interval = 2.0f;

    std::string GetTypeName() const override { return "FPSLogScript"; }

    void OnUpdate(float dt) override {
        m_timer += dt;
        if (m_timer >= interval) {
            m_timer = 0.0f;
            Log("FPS: " + std::to_string((int)Time::Get().FPS()));
        }
    }

private:
    float m_timer = 0.0f;
};

// -------------------------------------------------------
// PrintOnStartScript – logs a message on first frame
// -------------------------------------------------------
class PrintOnStartScript : public Script {
public:
    std::string message = "Hello from Script!";

    std::string GetTypeName() const override { return "PrintOnStartScript"; }

    void OnStart() override {
        Log(message);
    }
};

} // namespace Nova
