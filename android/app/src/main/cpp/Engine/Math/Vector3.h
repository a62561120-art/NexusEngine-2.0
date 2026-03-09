#pragma once
#include <cmath>
#include <string>

namespace Nova {

struct Vector3 {
    float x, y, z;

    Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    explicit Vector3(float v) : x(v), y(v), z(v) {}

    Vector3 operator+(const Vector3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vector3 operator-(const Vector3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vector3 operator*(float s)          const { return {x*s,   y*s,   z*s};   }
    Vector3 operator/(float s)          const { return {x/s,   y/s,   z/s};   }
    Vector3 operator-()                 const { return {-x, -y, -z}; }
    Vector3& operator+=(const Vector3& o) { x+=o.x; y+=o.y; z+=o.z; return *this; }
    Vector3& operator-=(const Vector3& o) { x-=o.x; y-=o.y; z-=o.z; return *this; }
    Vector3& operator*=(float s)          { x*=s;   y*=s;   z*=s;   return *this; }
    Vector3& operator/=(float s)          { x/=s;   y/=s;   z/=s;   return *this; }
    bool operator==(const Vector3& o) const { return x==o.x && y==o.y && z==o.z; }
    bool operator!=(const Vector3& o) const { return !(*this == o); }

    float Dot(const Vector3& o)   const { return x*o.x + y*o.y + z*o.z; }
    float LengthSq()              const { return x*x + y*y + z*z; }
    float Length()                const { return std::sqrt(LengthSq()); }
    Vector3 Normalized()          const { float l = Length(); return l > 1e-6f ? *this / l : Vector3(); }
    void Normalize()                    { *this = Normalized(); }

    Vector3 Cross(const Vector3& o) const {
        return {
            y * o.z - z * o.y,
            z * o.x - x * o.z,
            x * o.y - y * o.x
        };
    }

    std::string ToString() const {
        return "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")";
    }

    static Vector3 Zero()    { return {0,0,0}; }
    static Vector3 One()     { return {1,1,1}; }
    static Vector3 Up()      { return {0,1,0}; }
    static Vector3 Down()    { return {0,-1,0}; }
    static Vector3 Forward() { return {0,0,-1}; }
    static Vector3 Back()    { return {0,0,1}; }
    static Vector3 Right()   { return {1,0,0}; }
    static Vector3 Left()    { return {-1,0,0}; }

    static float Distance(const Vector3& a, const Vector3& b) { return (b - a).Length(); }
    static Vector3 Lerp(const Vector3& a, const Vector3& b, float t) { return a + (b - a) * t; }
    static Vector3 Reflect(const Vector3& v, const Vector3& n) { return v - n * 2.0f * v.Dot(n); }
};

inline Vector3 operator*(float s, const Vector3& v) { return v * s; }

} // namespace Nova
