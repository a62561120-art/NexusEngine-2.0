#pragma once
// ============================================================
//  Input.h
//  Keyboard and touch input system.
//  Works in both terminal mode (keyboard) and OpenGL ES mode.
// ============================================================
#include "../Math/Vector2.h"
#include <array>
#include <vector>
#include <string>
#include <functional>

namespace Nova {

// Key codes (subset matching common keys)
enum class Key {
    Unknown = 0,
    W, A, S, D,
    Q, E, R, F,
    Up, Down, Left, Right,
    Space, Escape, Enter,
    LeftShift, RightShift,
    LeftCtrl,  RightCtrl,
    Tab,
    Key0, Key1, Key2, Key3, Key4,
    Key5, Key6, Key7, Key8, Key9,
    COUNT
};

enum class MouseButton { Left = 0, Right = 1, Middle = 2 };

// Touch finger data
struct Touch {
    int     id;         // finger ID
    Vector2 position;   // screen position in pixels
    Vector2 delta;      // movement since last frame
    bool    active;
};

// -------------------------------------------------------
// Input – singleton that tracks keyboard, mouse and touch.
// Call Input::Get().Update() once per frame BEFORE reading.
// -------------------------------------------------------
class Input {
public:
    static Input& Get() {
        static Input instance;
        return instance;
    }

    // --- Called by the platform layer each frame ---
    void Update() {
        // Shift current state to previous
        m_prevKeys   = m_currKeys;
        m_prevMouse  = m_currMouse;

        // Clear per-frame deltas
        m_mouseDelta = Vector2::Zero();
        m_scrollDelta = 0.0f;

        // Reset touch deltas
        for (auto& t : m_touches) t.delta = Vector2::Zero();
    }

    // --- Keyboard ---
    void SetKey(Key k, bool pressed) {
        m_currKeys[(int)k] = pressed;
    }

    bool IsKeyDown(Key k)     const { return m_currKeys[(int)k]; }
    bool IsKeyUp(Key k)       const { return !m_currKeys[(int)k]; }
    bool IsKeyPressed(Key k)  const { // true only on the frame it was pressed
        return m_currKeys[(int)k] && !m_prevKeys[(int)k];
    }
    bool IsKeyReleased(Key k) const { // true only on the frame it was released
        return !m_currKeys[(int)k] && m_prevKeys[(int)k];
    }

    // --- Mouse ---
    void SetMousePosition(float x, float y) {
        m_mouseDelta = Vector2(x, y) - m_mousePos;
        m_mousePos   = Vector2(x, y);
    }
    void SetMouseButton(MouseButton b, bool pressed) {
        m_currMouse[(int)b] = pressed;
    }
    void SetScrollDelta(float delta) { m_scrollDelta = delta; }

    Vector2 GetMousePosition()  const { return m_mousePos; }
    Vector2 GetMouseDelta()     const { return m_mouseDelta; }
    float   GetScrollDelta()    const { return m_scrollDelta; }

    bool IsMouseDown(MouseButton b)     const { return m_currMouse[(int)b]; }
    bool IsMousePressed(MouseButton b)  const {
        return m_currMouse[(int)b] && !m_prevMouse[(int)b];
    }
    bool IsMouseReleased(MouseButton b) const {
        return !m_currMouse[(int)b] && m_prevMouse[(int)b];
    }

    // --- Touch ---
    void TouchBegin(int id, float x, float y) {
        Touch t; t.id = id; t.position = {x,y}; t.delta = {}; t.active = true;
        // Replace existing or add new
        for (auto& existing : m_touches) {
            if (existing.id == id) { existing = t; return; }
        }
        m_touches.push_back(t);
    }
    void TouchMove(int id, float x, float y) {
        for (auto& t : m_touches) {
            if (t.id == id) {
                t.delta    = Vector2(x, y) - t.position;
                t.position = {x, y};
                return;
            }
        }
    }
    void TouchEnd(int id) {
        for (auto& t : m_touches)
            if (t.id == id) { t.active = false; return; }
    }

    int   TouchCount() const {
        int n = 0;
        for (auto& t : m_touches) if (t.active) ++n;
        return n;
    }
    const Touch* GetTouch(int index) const {
        int n = 0;
        for (auto& t : m_touches) {
            if (t.active) {
                if (n == index) return &t;
                ++n;
            }
        }
        return nullptr;
    }

    // Convenience: primary touch position (first finger)
    Vector2 GetTouchPosition() const {
        const Touch* t = GetTouch(0);
        return t ? t->position : Vector2::Zero();
    }

    // Axis helpers (returns -1, 0 or 1)
    float GetHorizontal() const {
        float v = 0;
        if (IsKeyDown(Key::A) || IsKeyDown(Key::Left))  v -= 1.0f;
        if (IsKeyDown(Key::D) || IsKeyDown(Key::Right)) v += 1.0f;
        return v;
    }
    float GetVertical() const {
        float v = 0;
        if (IsKeyDown(Key::S) || IsKeyDown(Key::Down)) v -= 1.0f;
        if (IsKeyDown(Key::W) || IsKeyDown(Key::Up))   v += 1.0f;
        return v;
    }

    void Reset() {
        m_currKeys.fill(false);
        m_prevKeys.fill(false);
        m_currMouse.fill(false);
        m_prevMouse.fill(false);
        m_touches.clear();
        m_mousePos    = Vector2::Zero();
        m_mouseDelta  = Vector2::Zero();
        m_scrollDelta = 0.0f;
    }

private:
    Input() {
        m_currKeys.fill(false);
        m_prevKeys.fill(false);
        m_currMouse.fill(false);
        m_prevMouse.fill(false);
        m_scrollDelta = 0.0f;
    }

    std::array<bool, (int)Key::COUNT>  m_currKeys;
    std::array<bool, (int)Key::COUNT>  m_prevKeys;
    std::array<bool, 3>                m_currMouse;
    std::array<bool, 3>                m_prevMouse;
    Vector2                            m_mousePos;
    Vector2                            m_mouseDelta;
    float                              m_scrollDelta;
    std::vector<Touch>                 m_touches;
};

} // namespace Nova
