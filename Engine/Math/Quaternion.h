#pragma once
#include "Vector3.h"
#include <cmath>

namespace Nova {

struct Quaternion {
    float x, y, z, w;

    Quaternion() : x(0), y(0), z(0), w(1) {}
    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    Quaternion operator*(const Quaternion& o) const {
        return {
            w*o.x + x*o.w + y*o.z - z*o.y,
            w*o.y - x*o.z + y*o.w + z*o.x,
            w*o.z + x*o.y - y*o.x + z*o.w,
            w*o.w - x*o.x - y*o.y - z*o.z
        };
    }

    Quaternion& operator*=(const Quaternion& o) { *this = *this * o; return *this; }
    bool operator==(const Quaternion& o) const { return x==o.x && y==o.y && z==o.z && w==o.w; }

    float Length() const { return std::sqrt(x*x + y*y + z*z + w*w); }

    void Normalize() {
        float l = Length();
        if (l > 1e-6f) { x/=l; y/=l; z/=l; w/=l; }
    }

    Quaternion Normalized() const {
        Quaternion q = *this;
        q.Normalize();
        return q;
    }

    Quaternion Conjugate() const { return {-x, -y, -z, w}; }
    Quaternion Inverse()   const { return Conjugate(); } // assumes unit quaternion

    // Rotate a vector by this quaternion
    Vector3 RotateVector(const Vector3& v) const {
        Vector3 qv(x, y, z);
        Vector3 uv  = qv.Cross(v);
        Vector3 uuv = qv.Cross(uv);
        return v + ((uv * w) + uuv) * 2.0f;
    }

    // Convert to Euler angles (degrees): pitch (X), yaw (Y), roll (Z)
    Vector3 ToEulerDegrees() const {
        const float RAD2DEG = 180.0f / 3.14159265f;
        float sinr_cosp = 2.0f * (w*x + y*z);
        float cosr_cosp = 1.0f - 2.0f * (x*x + y*y);
        float pitch = std::atan2(sinr_cosp, cosr_cosp) * RAD2DEG;

        float sinp = 2.0f * (w*y - z*x);
        float yaw;
        if (std::fabs(sinp) >= 1.0f)
            yaw = std::copysign(90.0f, sinp);
        else
            yaw = std::asin(sinp) * RAD2DEG;

        float siny_cosp = 2.0f * (w*z + x*y);
        float cosy_cosp = 1.0f - 2.0f * (y*y + z*z);
        float roll = std::atan2(siny_cosp, cosy_cosp) * RAD2DEG;

        return {pitch, yaw, roll};
    }

    // Create from axis-angle (degrees)
    static Quaternion FromAxisAngle(const Vector3& axis, float degrees) {
        const float DEG2RAD = 3.14159265f / 180.0f;
        float half = degrees * DEG2RAD * 0.5f;
        float s = std::sin(half);
        Vector3 a = axis.Normalized();
        return {a.x * s, a.y * s, a.z * s, std::cos(half)};
    }

    // Create from Euler angles in degrees (applied: Y * X * Z)
    static Quaternion FromEulerDegrees(float pitch, float yaw, float roll) {
        Quaternion qx = FromAxisAngle({1,0,0}, pitch);
        Quaternion qy = FromAxisAngle({0,1,0}, yaw);
        Quaternion qz = FromAxisAngle({0,0,1}, roll);
        return (qy * qx * qz).Normalized();
    }

    static Quaternion FromEulerDegrees(const Vector3& euler) {
        return FromEulerDegrees(euler.x, euler.y, euler.z);
    }

    // Spherical linear interpolation
    static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t) {
        float dot = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
        Quaternion b2 = b;
        if (dot < 0.0f) {
            b2 = {-b.x, -b.y, -b.z, -b.w};
            dot = -dot;
        }
        if (dot > 0.9995f) {
            // Linear interpolation fallback
            Quaternion r = {
                a.x + t*(b2.x-a.x),
                a.y + t*(b2.y-a.y),
                a.z + t*(b2.z-a.z),
                a.w + t*(b2.w-a.w)
            };
            return r.Normalized();
        }
        float theta0 = std::acos(dot);
        float theta  = theta0 * t;
        float sinTheta  = std::sin(theta);
        float sinTheta0 = std::sin(theta0);
        float s0 = std::cos(theta) - dot * sinTheta / sinTheta0;
        float s1 = sinTheta / sinTheta0;
        return {
            s0*a.x + s1*b2.x,
            s0*a.y + s1*b2.y,
            s0*a.z + s1*b2.z,
            s0*a.w + s1*b2.w
        };
    }

    static Quaternion Identity() { return {0,0,0,1}; }
};

} // namespace Nova
