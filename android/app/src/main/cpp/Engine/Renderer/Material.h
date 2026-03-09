#pragma once
#include "../Math/Vector3.h"
#include "../Math/Vector4.h"
#include <string>

namespace Nova {

// -------------------------------------------------------
// Color – RGBA float color
// -------------------------------------------------------
struct Color {
    float r, g, b, a;

    Color() : r(1), g(1), b(1), a(1) {}
    Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}

    Vector3 ToVec3() const { return {r, g, b}; }
    Vector4 ToVec4() const { return {r, g, b, a}; }

    static Color White()   { return {1,1,1,1}; }
    static Color Black()   { return {0,0,0,1}; }
    static Color Red()     { return {1,0,0,1}; }
    static Color Green()   { return {0,1,0,1}; }
    static Color Blue()    { return {0,0,1,1}; }
    static Color Yellow()  { return {1,1,0,1}; }
    static Color Cyan()    { return {0,1,1,1}; }
    static Color Magenta() { return {1,0,1,1}; }
    static Color Gray()    { return {0.5f,0.5f,0.5f,1}; }
    static Color Clear()   { return {0,0,0,0}; }

    static Color Lerp(const Color& a, const Color& b, float t) {
        return {
            a.r + (b.r-a.r)*t,
            a.g + (b.g-a.g)*t,
            a.b + (b.b-a.b)*t,
            a.a + (b.a-a.a)*t
        };
    }
};

// -------------------------------------------------------
// Material – surface appearance properties
// -------------------------------------------------------
struct Material {
    std::string name = "Default";

    Color  albedo    = Color::White();
    Color  specular  = {0.5f, 0.5f, 0.5f, 1.0f};
    float  shininess = 32.0f;
    float  metallic  = 0.0f;
    float  roughness = 0.5f;

    // Flags
    bool   wireframe = false;
    bool   doubleSided = false;
    bool   transparent = false;

    // Texture IDs (0 = none; assigned by renderer)
    uint32_t albedoTexture   = 0;
    uint32_t normalTexture   = 0;
    uint32_t specularTexture = 0;

    // Shader override (0 = use default)
    uint32_t shaderID = 0;

    static Material Default() {
        Material m;
        m.name = "Default";
        return m;
    }

    static Material Colored(const Color& c) {
        Material m;
        m.albedo = c;
        return m;
    }
};

} // namespace Nova
