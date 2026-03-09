#pragma once
#include <cmath>
#include <string>

namespace Nova {

struct Vector2 {
    float x, y;

    Vector2() : x(0.0f), y(0.0f) {}
    Vector2(float x, float y) : x(x), y(y) {}

    Vector2 operator+(const Vector2& o) const { return {x + o.x, y + o.y}; }
    Vector2 operator-(const Vector2& o) const { return {x - o.x, y - o.y}; }
    Vector2 operator*(float s)          const { return {x * s,   y * s};   }
    Vector2 operator/(float s)          const { return {x / s,   y / s};   }
    Vector2& operator+=(const Vector2& o) { x += o.x; y += o.y; return *this; }
    Vector2& operator-=(const Vector2& o) { x -= o.x; y -= o.y; return *this; }
    Vector2& operator*=(float s)          { x *= s;   y *= s;   return *this; }
    bool operator==(const Vector2& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Vector2& o) const { return !(*this == o); }

    float Dot(const Vector2& o)   const { return x * o.x + y * o.y; }
    float LengthSq()              const { return x * x + y * y; }
    float Length()                const { return std::sqrt(LengthSq()); }
    Vector2 Normalized()          const { float l = Length(); return l > 0 ? *this / l : Vector2(); }
    void Normalize()                    { *this = Normalized(); }

    std::string ToString() const {
        return "(" + std::to_string(x) + ", " + std::to_string(y) + ")";
    }

    static Vector2 Zero()  { return {0, 0}; }
    static Vector2 One()   { return {1, 1}; }
    static Vector2 Up()    { return {0, 1}; }
    static Vector2 Right() { return {1, 0}; }
    static float Distance(const Vector2& a, const Vector2& b) { return (b - a).Length(); }
    static Vector2 Lerp(const Vector2& a, const Vector2& b, float t) { return a + (b - a) * t; }
};

} // namespace Nova
