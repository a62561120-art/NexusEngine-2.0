#pragma once
#include "Vector3.h"
#include "Vector4.h"
#include "Quaternion.h"
#include <cmath>
#include <cstring>

namespace Nova {

// Column-major 4x4 matrix (OpenGL convention)
struct Matrix4 {
    float m[16]; // m[col * 4 + row]

    Matrix4() { SetIdentity(); }

    void SetIdentity() {
        memset(m, 0, sizeof(m));
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    float& At(int row, int col)             { return m[col*4 + row]; }
    float  At(int row, int col) const       { return m[col*4 + row]; }

    Matrix4 operator*(const Matrix4& o) const {
        Matrix4 r;
        for (int row = 0; row < 4; ++row)
            for (int col = 0; col < 4; ++col) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k)
                    sum += At(row, k) * o.At(k, col);
                r.At(row, col) = sum;
            }
        return r;
    }

    Vector4 operator*(const Vector4& v) const {
        return {
            At(0,0)*v.x + At(0,1)*v.y + At(0,2)*v.z + At(0,3)*v.w,
            At(1,0)*v.x + At(1,1)*v.y + At(1,2)*v.z + At(1,3)*v.w,
            At(2,0)*v.x + At(2,1)*v.y + At(2,2)*v.z + At(2,3)*v.w,
            At(3,0)*v.x + At(3,1)*v.y + At(3,2)*v.z + At(3,3)*v.w
        };
    }

    // Transform a point (w=1)
    Vector3 TransformPoint(const Vector3& p) const {
        Vector4 r = *this * Vector4(p.x, p.y, p.z, 1.0f);
        return {r.x, r.y, r.z};
    }

    // Transform a direction (w=0)
    Vector3 TransformDirection(const Vector3& d) const {
        Vector4 r = *this * Vector4(d.x, d.y, d.z, 0.0f);
        return {r.x, r.y, r.z};
    }

    // --- Factory methods ---

    static Matrix4 Identity() { return Matrix4(); }

    static Matrix4 Translation(float tx, float ty, float tz) {
        Matrix4 r;
        r.At(0,3) = tx;
        r.At(1,3) = ty;
        r.At(2,3) = tz;
        return r;
    }

    static Matrix4 Translation(const Vector3& t) {
        return Translation(t.x, t.y, t.z);
    }

    static Matrix4 Scale(float sx, float sy, float sz) {
        Matrix4 r;
        r.At(0,0) = sx;
        r.At(1,1) = sy;
        r.At(2,2) = sz;
        return r;
    }

    static Matrix4 Scale(const Vector3& s) {
        return Scale(s.x, s.y, s.z);
    }

    static Matrix4 RotationX(float deg) {
        const float DEG2RAD = 3.14159265f / 180.0f;
        float c = std::cos(deg * DEG2RAD), s = std::sin(deg * DEG2RAD);
        Matrix4 r;
        r.At(1,1)= c; r.At(1,2)=-s;
        r.At(2,1)= s; r.At(2,2)= c;
        return r;
    }

    static Matrix4 RotationY(float deg) {
        const float DEG2RAD = 3.14159265f / 180.0f;
        float c = std::cos(deg * DEG2RAD), s = std::sin(deg * DEG2RAD);
        Matrix4 r;
        r.At(0,0)= c; r.At(0,2)= s;
        r.At(2,0)=-s; r.At(2,2)= c;
        return r;
    }

    static Matrix4 RotationZ(float deg) {
        const float DEG2RAD = 3.14159265f / 180.0f;
        float c = std::cos(deg * DEG2RAD), s = std::sin(deg * DEG2RAD);
        Matrix4 r;
        r.At(0,0)= c; r.At(0,1)=-s;
        r.At(1,0)= s; r.At(1,1)= c;
        return r;
    }

    // Build TRS matrix from quaternion rotation
    static Matrix4 FromTRS(const Vector3& pos, const Quaternion& rot, const Vector3& scale) {
        float qx=rot.x, qy=rot.y, qz=rot.z, qw=rot.w;
        Matrix4 r;
        r.At(0,0) = (1 - 2*(qy*qy + qz*qz)) * scale.x;
        r.At(1,0) = (2*(qx*qy + qz*qw))     * scale.x;
        r.At(2,0) = (2*(qx*qz - qy*qw))     * scale.x;
        r.At(0,1) = (2*(qx*qy - qz*qw))     * scale.y;
        r.At(1,1) = (1 - 2*(qx*qx + qz*qz)) * scale.y;
        r.At(2,1) = (2*(qy*qz + qx*qw))     * scale.y;
        r.At(0,2) = (2*(qx*qz + qy*qw))     * scale.z;
        r.At(1,2) = (2*(qy*qz - qx*qw))     * scale.z;
        r.At(2,2) = (1 - 2*(qx*qx + qy*qy)) * scale.z;
        r.At(0,3) = pos.x;
        r.At(1,3) = pos.y;
        r.At(2,3) = pos.z;
        r.At(3,3) = 1.0f;
        return r;
    }

    static Matrix4 LookAt(const Vector3& eye, const Vector3& center, const Vector3& up) {
        Vector3 f = (center - eye).Normalized();
        Vector3 r = f.Cross(up).Normalized();
        Vector3 u = r.Cross(f);
        Matrix4 mat;
        mat.At(0,0)= r.x; mat.At(0,1)= r.y; mat.At(0,2)= r.z;
        mat.At(1,0)= u.x; mat.At(1,1)= u.y; mat.At(1,2)= u.z;
        mat.At(2,0)=-f.x; mat.At(2,1)=-f.y; mat.At(2,2)=-f.z;
        mat.At(0,3)= -r.Dot(eye);
        mat.At(1,3)= -u.Dot(eye);
        mat.At(2,3)=  f.Dot(eye);
        mat.At(3,3)= 1.0f;
        return mat;
    }

    static Matrix4 Perspective(float fovDeg, float aspect, float nearZ, float farZ) {
        const float DEG2RAD = 3.14159265f / 180.0f;
        float tanHalf = std::tan(fovDeg * DEG2RAD * 0.5f);
        Matrix4 r;
        r.SetIdentity();
        memset(r.m, 0, sizeof(r.m));
        r.At(0,0) = 1.0f / (aspect * tanHalf);
        r.At(1,1) = 1.0f / tanHalf;
        r.At(2,2) = -(farZ + nearZ) / (farZ - nearZ);
        r.At(3,2) = -1.0f;
        r.At(2,3) = -(2.0f * farZ * nearZ) / (farZ - nearZ);
        return r;
    }

    static Matrix4 Orthographic(float left, float right, float bottom, float top, float nearZ, float farZ) {
        Matrix4 r;
        r.At(0,0) =  2.0f / (right - left);
        r.At(1,1) =  2.0f / (top - bottom);
        r.At(2,2) = -2.0f / (farZ - nearZ);
        r.At(0,3) = -(right + left) / (right - left);
        r.At(1,3) = -(top + bottom) / (top - bottom);
        r.At(2,3) = -(farZ + nearZ) / (farZ - nearZ);
        r.At(3,3) =  1.0f;
        return r;
    }

    // Inverse (assumes TRS matrix for efficiency)
    Matrix4 Transposed() const {
        Matrix4 r;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                r.At(i,j) = At(j,i);
        return r;
    }

    // Full inverse (Gauss-Jordan)
    Matrix4 Inverse() const {
        float inv[16], det;
        const float* src = m;
        inv[0]  =  src[5]*src[10]*src[15] - src[5]*src[11]*src[14] - src[9]*src[6]*src[15] + src[9]*src[7]*src[14] + src[13]*src[6]*src[11] - src[13]*src[7]*src[10];
        inv[4]  = -src[4]*src[10]*src[15] + src[4]*src[11]*src[14] + src[8]*src[6]*src[15] - src[8]*src[7]*src[14] - src[12]*src[6]*src[11] + src[12]*src[7]*src[10];
        inv[8]  =  src[4]*src[9]*src[15]  - src[4]*src[11]*src[13] - src[8]*src[5]*src[15] + src[8]*src[7]*src[13] + src[12]*src[5]*src[11] - src[12]*src[7]*src[9];
        inv[12] = -src[4]*src[9]*src[14]  + src[4]*src[10]*src[13] + src[8]*src[5]*src[14] - src[8]*src[6]*src[13] - src[12]*src[5]*src[10] + src[12]*src[6]*src[9];
        inv[1]  = -src[1]*src[10]*src[15] + src[1]*src[11]*src[14] + src[9]*src[2]*src[15] - src[9]*src[3]*src[14] - src[13]*src[2]*src[11] + src[13]*src[3]*src[10];
        inv[5]  =  src[0]*src[10]*src[15] - src[0]*src[11]*src[14] - src[8]*src[2]*src[15] + src[8]*src[3]*src[14] + src[12]*src[2]*src[11] - src[12]*src[3]*src[10];
        inv[9]  = -src[0]*src[9]*src[15]  + src[0]*src[11]*src[13] + src[8]*src[1]*src[15] - src[8]*src[3]*src[13] - src[12]*src[1]*src[11] + src[12]*src[3]*src[9];
        inv[13] =  src[0]*src[9]*src[14]  - src[0]*src[10]*src[13] - src[8]*src[1]*src[14] + src[8]*src[2]*src[13] + src[12]*src[1]*src[10] - src[12]*src[2]*src[9];
        inv[2]  =  src[1]*src[6]*src[15]  - src[1]*src[7]*src[14]  - src[5]*src[2]*src[15] + src[5]*src[3]*src[14] + src[13]*src[2]*src[7]  - src[13]*src[3]*src[6];
        inv[6]  = -src[0]*src[6]*src[15]  + src[0]*src[7]*src[14]  + src[4]*src[2]*src[15] - src[4]*src[3]*src[14] - src[12]*src[2]*src[7]  + src[12]*src[3]*src[6];
        inv[10] =  src[0]*src[5]*src[15]  - src[0]*src[7]*src[13]  - src[4]*src[1]*src[15] + src[4]*src[3]*src[13] + src[12]*src[1]*src[7]  - src[12]*src[3]*src[5];
        inv[14] = -src[0]*src[5]*src[14]  + src[0]*src[6]*src[13]  + src[4]*src[1]*src[14] - src[4]*src[2]*src[13] - src[12]*src[1]*src[6]  + src[12]*src[2]*src[5];
        inv[3]  = -src[1]*src[6]*src[11]  + src[1]*src[7]*src[10]  + src[5]*src[2]*src[11] - src[5]*src[3]*src[10] - src[9]*src[2]*src[7]   + src[9]*src[3]*src[6];
        inv[7]  =  src[0]*src[6]*src[11]  - src[0]*src[7]*src[10]  - src[4]*src[2]*src[11] + src[4]*src[3]*src[10] + src[8]*src[2]*src[7]   - src[8]*src[3]*src[6];
        inv[11] = -src[0]*src[5]*src[11]  + src[0]*src[7]*src[9]   + src[4]*src[1]*src[11] - src[4]*src[3]*src[9]  - src[8]*src[1]*src[7]   + src[8]*src[3]*src[5];
        inv[15] =  src[0]*src[5]*src[10]  - src[0]*src[6]*src[9]   - src[4]*src[1]*src[10] + src[4]*src[2]*src[9]  + src[8]*src[1]*src[6]   - src[8]*src[2]*src[5];
        det = src[0]*inv[0] + src[1]*inv[4] + src[2]*inv[8] + src[3]*inv[12];
        if (std::fabs(det) < 1e-8f) return Identity();
        det = 1.0f / det;
        Matrix4 r;
        for (int i = 0; i < 16; i++) r.m[i] = inv[i] * det;
        return r;
    }
};

} // namespace Nova
