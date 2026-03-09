#pragma once
#include <string>
#include <unordered_map>
#include "../Math/Vector3.h"
#include "../Math/Vector4.h"
#include "../Math/Matrix4.h"
#include "../Core/Logger.h"

namespace Nova {

// -------------------------------------------------------
// ShaderSource – plain text GLSL shader source
// -------------------------------------------------------
struct ShaderSource {
    std::string vertexSrc;
    std::string fragmentSrc;
};

// -------------------------------------------------------
// Built-in shader sources (GLSL ES 1.00 compatible
// so they work on both OpenGL ES and desktop OpenGL)
// -------------------------------------------------------
namespace BuiltinShaders {

// ---- Phong / Lambert shading ----
inline const char* PHONG_VERT = R"(
attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec2 a_uv;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;
uniform mat3 u_normalMatrix;

varying vec3 v_worldPos;
varying vec3 v_normal;
varying vec2 v_uv;

void main() {
    vec4 worldPos = u_model * vec4(a_position, 1.0);
    v_worldPos = worldPos.xyz;
    v_normal   = normalize(u_normalMatrix * a_normal);
    v_uv       = a_uv;
    gl_Position = u_proj * u_view * worldPos;
}
)";

inline const char* PHONG_FRAG = R"(
precision mediump float;

uniform vec3  u_albedo;
uniform vec3  u_lightDir;
uniform vec3  u_lightColor;
uniform float u_lightIntensity;
uniform vec3  u_ambientColor;
uniform vec3  u_specular;
uniform float u_shininess;
uniform vec3  u_cameraPos;

varying vec3 v_worldPos;
varying vec3 v_normal;
varying vec2 v_uv;

void main() {
    vec3 N = normalize(v_normal);
    vec3 L = normalize(-u_lightDir);
    vec3 V = normalize(u_cameraPos - v_worldPos);
    vec3 R = reflect(-L, N);

    // Ambient
    vec3 ambient = u_ambientColor * u_albedo;

    // Diffuse
    float diff   = max(dot(N, L), 0.0);
    vec3 diffuse = diff * u_lightColor * u_lightIntensity * u_albedo;

    // Specular (Phong)
    float spec   = pow(max(dot(V, R), 0.0), u_shininess);
    vec3 specular= spec * u_specular * u_lightColor;

    vec3 color = ambient + diffuse + specular;
    gl_FragColor = vec4(color, 1.0);
}
)";

// ---- Unlit colour ----
inline const char* UNLIT_VERT = R"(
attribute vec3 a_position;
uniform mat4 u_mvp;
void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
)";

inline const char* UNLIT_FRAG = R"(
precision mediump float;
uniform vec4 u_color;
void main() {
    gl_FragColor = u_color;
}
)";

// ---- Wireframe (same vert, different frag intent) ----
inline const char* WIREFRAME_FRAG = R"(
precision mediump float;
uniform vec4 u_color;
void main() {
    gl_FragColor = u_color;
}
)";

} // namespace BuiltinShaders

// -------------------------------------------------------
// Shader – wraps an OpenGL program.
// Actual compilation done by the Renderer (GL context required).
// -------------------------------------------------------
class Shader {
public:
    Shader() : m_programID(0), m_valid(false) {}
    virtual ~Shader() {}

    bool   IsValid()     const { return m_valid; }
    uint32_t GetID()     const { return m_programID; }
    const std::string& GetName() const { return m_name; }

    // These are set by the Renderer after compilation
    void SetID(uint32_t id)       { m_programID = id; }
    void SetValid(bool v)         { m_valid = v; }
    void SetName(const std::string& n) { m_name = n; }

    // Cache uniform locations
    int GetUniformLocation(const std::string& name) const {
        auto it = m_uniformCache.find(name);
        if (it != m_uniformCache.end()) return it->second;
        return -1;
    }
    void CacheUniform(const std::string& name, int loc) {
        m_uniformCache[name] = loc;
    }

protected:
    uint32_t    m_programID;
    bool        m_valid;
    std::string m_name;
    mutable std::unordered_map<std::string, int> m_uniformCache;
};

} // namespace Nova
