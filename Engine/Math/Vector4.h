#pragma once
#include <cmath>
#include <string>

namespace Nova {

struct Vector4 {
    float x, y, z, w;

    Vector4() : x(0), y(0), z(0), w(0) {}
    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    Vector4 operator+(const Vector4& o) const { return {x+o.x, y+o.y, z+o.z, w+o.w}; }
    Vector4 operator-(const Vector4& o) const { return {x-o.x, y-o.y, z-o.z, w-o.w}; }
    Vector4 operator*(float s)          const { return {x*s, y*s, z*s, w*s}; }
    float   Dot(const Vector4& o)       const { return x*o.x + y*o.y + z*o.z + w*o.w; }
    float   Length()                    const { return std::sqrt(x*x+y*y+z*z+w*w); }

    std::string ToString() const {
        return "(" + std::to_string(x) + ", " + std::to_string(y) +
               ", " + std::to_string(z) + ", " + std::to_string(w) + ")";
    }

    static Vector4 Zero() { return {0,0,0,0}; }
    static Vector4 One()  { return {1,1,1,1}; }
};

} // namespace Nova
