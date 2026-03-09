#pragma once
// ============================================================
//  EditorCamera.h  —  Mobile-Friendly Editor Camera
// ============================================================
#include <GLFW/glfw3.h>
#include "../Renderer/Camera.h"
#include "../Core/Transform.h"
#include "../Core/GameObject.h"
#include "../Math/Vector3.h"
#include <cmath>

namespace Nova {

class EditorCamera {
public:
    EditorCamera()
        : m_go(nullptr), m_cam(nullptr),
          m_yaw(-90.0f), m_pitch(-20.0f),
          m_lastX(0), m_lastY(0),
          m_rightHeld(false), m_middleHeld(false), m_leftHeld(false),
          m_moveSpeed(5.0f), m_lookSensitivity(0.25f),
          m_moveForward(false), m_moveBack(false),
          m_moveLeft(false), m_moveRight(false),
          m_moveUp(false), m_moveDown(false),
          m_lookLeft(false), m_lookRight(false),
          m_lookUp(false), m_lookDown(false)
    {}

    void Init(Scene* scene, float aspect) {
        m_go  = scene->CreateGameObject("EditorCamera");
        m_cam = m_go->AddComponent<Camera>();
        m_cam->SetFOV(60.0f);
        m_cam->SetAspect(aspect);
        m_cam->SetNear(0.05f);
        m_cam->SetFar(500.0f);
        m_go->GetTransform()->SetPosition({0, 4, 10});
        ApplyRotation();
    }

    Camera*     GetCamera()     const { return m_cam; }
    GameObject* GetGameObject() const { return m_go;  }

    void Update(GLFWwindow* window, float dt, bool uiWantsInput) {
        float spd = m_moveSpeed * dt;

        if (!uiWantsInput && window) {
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) spd *= 3.0f;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) m_moveForward = true;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) m_moveBack    = true;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) m_moveLeft    = true;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) m_moveRight   = true;
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) m_moveUp      = true;
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) m_moveDown    = true;
        }

        Vector3 fwd   = GetForward();
        Vector3 right = GetRight();
        Vector3 pos   = m_go->GetTransform()->GetPosition();

        if (m_moveForward) pos = pos + fwd   *  spd;
        if (m_moveBack)    pos = pos - fwd   *  spd;
        if (m_moveLeft)    pos = pos - right *  spd;
        if (m_moveRight)   pos = pos + right *  spd;
        if (m_moveUp)      pos.y += spd;
        if (m_moveDown)    pos.y -= spd;

        m_go->GetTransform()->SetPosition(pos);

        float lookSpd = 60.0f * dt;
        if (m_lookLeft)  { m_yaw   -= lookSpd; ApplyRotation(); }
        if (m_lookRight) { m_yaw   += lookSpd; ApplyRotation(); }
        if (m_lookUp)    { m_pitch  = std::min(m_pitch + lookSpd, 89.0f); ApplyRotation(); }
        if (m_lookDown)  { m_pitch  = std::max(m_pitch - lookSpd,-89.0f); ApplyRotation(); }

        m_moveForward = m_moveBack  = m_moveLeft  = m_moveRight = false;
        m_moveUp      = m_moveDown  = false;
        m_lookLeft    = m_lookRight = m_lookUp    = m_lookDown  = false;
    }

    void OnMouseButton(int button, int action, bool uiWantsInput) {
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
            m_rightHeld  = (action == GLFW_PRESS) && !uiWantsInput;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE)
            m_middleHeld = (action == GLFW_PRESS) && !uiWantsInput;
        // Left click drag on viewport = look around (mobile friendly!)
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            m_leftHeld = (action == GLFW_PRESS) && !uiWantsInput;
    }

    void OnMouseMove(double x, double y) {
        double dx = x - m_lastX, dy = y - m_lastY;
        m_lastX = x; m_lastY = y;
        // Right click drag OR left click drag on viewport = look around
        if (m_rightHeld || m_leftHeld) {
            m_yaw   += (float)dx * m_lookSensitivity;
            m_pitch -= (float)dy * m_lookSensitivity;
            m_pitch  = std::max(-89.0f, std::min(89.0f, m_pitch));
            ApplyRotation();
        }
        if (m_middleHeld) {
            Vector3 pos = m_go->GetTransform()->GetPosition();
            pos = pos - GetRight()         * (float)dx * 0.02f;
            pos = pos + Vector3(0,1,0)     * (float)dy * 0.02f;
            m_go->GetTransform()->SetPosition(pos);
        }
    }

    void OnScroll(double dy, bool uiWantsInput) {
        if (uiWantsInput) return;
        Vector3 pos = m_go->GetTransform()->GetPosition();
        pos = pos + GetForward() * (float)dy * 0.8f;
        m_go->GetTransform()->SetPosition(pos);
    }

    void FocusOn(GameObject* target) {
        if (!target) return;
        Vector3 tp = target->GetTransform()->GetWorldPosition();
        m_go->GetTransform()->SetPosition(tp - GetForward() * 5.0f);
    }

    void SetAspect(float a)          { if (m_cam) m_cam->SetAspect(a); }
    void SetMoveSpeed(float s)       { m_moveSpeed = s; }
    void SetLookSensitivity(float s) { m_lookSensitivity = s; }
    float GetMoveSpeed()       const { return m_moveSpeed; }
    float GetLookSensitivity() const { return m_lookSensitivity; }

    void SetMoveForward(bool v) { m_moveForward = v; }
    void SetMoveBack   (bool v) { m_moveBack    = v; }
    void SetMoveLeft   (bool v) { m_moveLeft    = v; }
    void SetMoveRight  (bool v) { m_moveRight   = v; }
    void SetMoveUp     (bool v) { m_moveUp      = v; }
    void SetMoveDown   (bool v) { m_moveDown    = v; }
    void SetLookLeft   (bool v) { m_lookLeft    = v; }
    void SetLookRight  (bool v) { m_lookRight   = v; }
    void SetLookUp     (bool v) { m_lookUp      = v; }
    void SetLookDown   (bool v) { m_lookDown    = v; }

private:
    void ApplyRotation() {
        m_go->GetTransform()->SetEulerAngles({m_pitch, m_yaw, 0});
    }
    Vector3 GetForward() const {
        const float D2R = 3.14159265f / 180.0f;
        float yR = m_yaw*D2R, pR = m_pitch*D2R;
        return Vector3(std::cos(pR)*std::cos(yR), std::sin(pR), std::cos(pR)*std::sin(yR)).Normalized();
    }
    Vector3 GetRight() const { return GetForward().Cross({0,1,0}).Normalized(); }

    GameObject* m_go;
    Camera*     m_cam;
    float       m_yaw, m_pitch;
    double      m_lastX, m_lastY;
    bool        m_rightHeld, m_middleHeld, m_leftHeld;
    float       m_moveSpeed, m_lookSensitivity;
    bool m_moveForward, m_moveBack, m_moveLeft, m_moveRight;
    bool m_moveUp, m_moveDown;
    bool m_lookLeft, m_lookRight, m_lookUp, m_lookDown;
};

} // namespace Nova
