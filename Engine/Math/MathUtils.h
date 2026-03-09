#pragma once
#include <cmath>
#include <algorithm>

namespace Nova {
namespace Math {

constexpr float PI         = 3.14159265358979323846f;
constexpr float TWO_PI     = PI * 2.0f;
constexpr float HALF_PI    = PI * 0.5f;
constexpr float DEG2RAD    = PI / 180.0f;
constexpr float RAD2DEG    = 180.0f / PI;
constexpr float EPSILON    = 1e-6f;
constexpr float INFINITY_F = 1e30f;

inline float ToRadians(float deg) { return deg * DEG2RAD; }
inline float ToDegrees(float rad) { return rad * RAD2DEG; }

inline float Clamp(float v, float lo, float hi) {
    return std::max(lo, std::min(v, hi));
}

inline float Clamp01(float v) { return Clamp(v, 0.0f, 1.0f); }

inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }

inline float Smoothstep(float edge0, float edge1, float x) {
    float t = Clamp01((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

inline bool NearlyEqual(float a, float b, float eps = EPSILON) {
    return std::fabs(a - b) < eps;
}

inline float Sign(float v) { return v >= 0.0f ? 1.0f : -1.0f; }
inline float Abs(float v)  { return std::fabs(v); }
inline float Sqrt(float v) { return std::sqrt(v); }

inline float PingPong(float t, float length) {
    t = std::fmod(t, length * 2.0f);
    return length - std::fabs(t - length);
}

inline float Repeat(float t, float length) {
    return t - std::floor(t / length) * length;
}

inline float InverseLerp(float a, float b, float v) {
    return (b != a) ? Clamp01((v - a) / (b - a)) : 0.0f;
}

inline int NextPowerOfTwo(int v) {
    v--;
    v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16;
    return v + 1;
}

} // namespace Math
} // namespace Nova
