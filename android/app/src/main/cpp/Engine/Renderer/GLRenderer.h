#pragma once
// ============================================================
//  GLRenderer.h  — OpenGL 3.3 Core Profile Renderer
//  Works with GLFW + GLEW on Termux/VNC
// ============================================================
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Camera.h"
#include "Light.h"
#include "MeshRenderer.h"
#include "Shader.h"
#include "../Scene/Scene.h"
#include "../Core/Logger.h"
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cstring>

namespace Nova {

// ---- Built-in GLSL 3.30 shaders ----
namespace GL3Shaders {

inline const char* PHONG_VERT = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;
uniform mat3 u_normalMatrix;

out vec3 v_worldPos;
out vec3 v_normal;
out vec2 v_uv;

void main() {
    vec4 worldPos = u_model * vec4(a_position, 1.0);
    v_worldPos = worldPos.xyz;
    v_normal   = normalize(u_normalMatrix * a_normal);
    v_uv       = a_uv;
    gl_Position = u_proj * u_view * worldPos;
}
)";

inline const char* PHONG_FRAG = R"(
#version 330 core
in vec3 v_worldPos;
in vec3 v_normal;
in vec2 v_uv;

uniform vec3  u_albedo;
uniform vec3  u_specular;
uniform float u_shininess;
uniform vec3  u_lightDir;
uniform vec3  u_lightColor;
uniform float u_lightIntensity;
uniform vec3  u_ambientColor;
uniform vec3  u_cameraPos;

out vec4 fragColor;

void main() {
    vec3 N = normalize(v_normal);
    vec3 L = normalize(-u_lightDir);
    vec3 V = normalize(u_cameraPos - v_worldPos);
    vec3 R = reflect(-L, N);

    vec3 ambient  = u_ambientColor * u_albedo;
    float diff    = max(dot(N, L), 0.0);
    vec3 diffuse  = diff * u_lightColor * u_lightIntensity * u_albedo;
    float spec    = pow(max(dot(V, R), 0.0), u_shininess);
    vec3 specular = spec * u_specular * u_lightColor;

    fragColor = vec4(ambient + diffuse + specular, 1.0);
}
)";

inline const char* UNLIT_VERT = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
uniform mat4 u_mvp;
void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
)";

inline const char* UNLIT_FRAG = R"(
#version 330 core
uniform vec4 u_color;
out vec4 fragColor;
void main() { fragColor = u_color; }
)";

// Wireframe / outline shader
inline const char* OUTLINE_VERT = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;
uniform float u_outlineSize;
void main() {
    vec3 pos = a_position + a_normal * u_outlineSize;
    gl_Position = u_proj * u_view * u_model * vec4(pos, 1.0);
}
)";

inline const char* OUTLINE_FRAG = R"(
#version 330 core
uniform vec4 u_color;
out vec4 fragColor;
void main() { fragColor = u_color; }
)";

} // namespace GL3Shaders

// GPU mesh buffers
struct GLMesh {
    GLuint vao=0, vbo=0, ebo=0;
    GLsizei indexCount=0;
    bool valid=false;
};

struct RenderCmd {
    MeshRenderer* mr;
    float dist;
    bool selected;
};

// -------------------------------------------------------
// GLRenderer
// -------------------------------------------------------
class GLRenderer {
public:
    static GLRenderer& Get() {
        static GLRenderer inst;
        return inst;
    }

    bool Init(int w, int h) {
        m_vpW=w; m_vpH=h;

        // Init GLEW
        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        if (err != GLEW_OK) {
            NOVA_LOG_ERROR("GLRenderer","GLEW init failed: " +
                std::string((char*)glewGetErrorString(err)));
            return false;
        }

        glViewport(0,0,w,h);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Compile shaders
        m_phong   = CompileProgram("Phong",
            GL3Shaders::PHONG_VERT,   GL3Shaders::PHONG_FRAG);
        m_unlit   = CompileProgram("Unlit",
            GL3Shaders::UNLIT_VERT,   GL3Shaders::UNLIT_FRAG);
        m_outline = CompileProgram("Outline",
            GL3Shaders::OUTLINE_VERT, GL3Shaders::OUTLINE_FRAG);

        BuildGrid(20, 1.0f);

        NOVA_LOG_INFO("GLRenderer","Ready " +
            std::to_string(w)+"x"+std::to_string(h));
        return true;
    }

    void SetViewport(int w, int h) {
        m_vpW=w; m_vpH=h;
        glViewport(0,0,w,h);
        if (m_camera) m_camera->SetAspect((float)w/h);
    }

    void SetActiveCamera(Camera* c)       { m_camera=c; }
    Camera* GetActiveCamera()       const { return m_camera; }
    void SetAmbient(float r,float g,float b){ m_ambient={r,g,b}; }
    void SetShowGrid(bool v)              { m_showGrid=v; }
    void SetSelectedObject(GameObject* g) { m_selected=g; }

    void BeginFrame(float r,float g,float b) {
        glClearColor(r,g,b,1.0f);
        glClearDepth(1.0);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    }

    void RenderScene(Scene* scene) {
        if (!scene||!m_camera) return;
        Matrix4 view   = m_camera->GetViewMatrix();
        Matrix4 proj   = m_camera->GetProjectionMatrix();
        Vector3 camPos = m_camera->GetPosition();

        // Collect lights
        m_lights.clear();
        scene->ForEach([this](GameObject* go){
            if (!go->IsActive()) return;
            Light* l=go->GetComponent<Light>();
            if (l&&l->IsEnabled()) m_lights.push_back(l);
        });

        // Collect render commands
        m_cmds.clear();
        scene->ForEach([&](GameObject* go){
            if (!go->IsActive()) return;
            MeshRenderer* mr=go->GetComponent<MeshRenderer>();
            if (mr&&mr->IsEnabled()&&mr->HasMesh()) {
                float d=Vector3::Distance(camPos,
                    go->GetTransform()->GetWorldPosition());
                m_cmds.push_back({mr,d,go==m_selected});
            }
        });

        // Sort opaque front-to-back
        std::sort(m_cmds.begin(),m_cmds.end(),
            [](const RenderCmd& a,const RenderCmd& b){
                return a.dist<b.dist;});

        // Draw opaque
        for (auto& c:m_cmds)
            if (!c.mr->GetMaterial().transparent)
                DrawMesh(c.mr,view,proj,camPos,c.selected);

        // Draw transparent back-to-front
        std::sort(m_cmds.begin(),m_cmds.end(),
            [](const RenderCmd& a,const RenderCmd& b){
                return a.dist>b.dist;});
        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);
        for (auto& c:m_cmds)
            if (c.mr->GetMaterial().transparent)
                DrawMesh(c.mr,view,proj,camPos,c.selected);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        if (m_showGrid) DrawGrid(view,proj);
    }

    // Upload mesh, return key
    uint32_t UploadMesh(const MeshData& data) {
        GLMesh gpu;
        glGenVertexArrays(1,&gpu.vao);
        glGenBuffers(1,&gpu.vbo);
        glGenBuffers(1,&gpu.ebo);

        glBindVertexArray(gpu.vao);
        glBindBuffer(GL_ARRAY_BUFFER,gpu.vbo);
        glBufferData(GL_ARRAY_BUFFER,
            data.vertices.size()*sizeof(Vertex),
            data.vertices.data(),GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,gpu.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            data.indices.size()*sizeof(uint32_t),
            data.indices.data(),GL_STATIC_DRAW);

        int stride=sizeof(Vertex);
        // position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,stride,
            (void*)offsetof(Vertex,position));
        // normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,stride,
            (void*)offsetof(Vertex,normal));
        // uv
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,stride,
            (void*)offsetof(Vertex,uv));

        glBindVertexArray(0);
        gpu.indexCount=(GLsizei)data.indices.size();
        gpu.valid=true;

        uint32_t key=m_nextKey++;
        m_meshes[key]=gpu;
        return key;
    }

    void FreeMesh(uint32_t key) {
        auto it=m_meshes.find(key);
        if (it==m_meshes.end()) return;
        glDeleteVertexArrays(1,&it->second.vao);
        glDeleteBuffers(1,&it->second.vbo);
        glDeleteBuffers(1,&it->second.ebo);
        m_meshes.erase(it);
    }

    int GetWidth()  const { return m_vpW; }
    int GetHeight() const { return m_vpH; }

private:
    GLRenderer()
        : m_camera(nullptr), m_selected(nullptr),
          m_ambient({0.1f,0.1f,0.15f}),
          m_vpW(1280),m_vpH(720),m_nextKey(1),
          m_showGrid(true),m_phong(0),m_unlit(0),m_outline(0),
          m_gridVAO(0),m_gridVBO(0),m_gridVertCount(0)
    {}

    void DrawMesh(MeshRenderer* mr, const Matrix4& view,
                  const Matrix4& proj, const Vector3& camPos,
                  bool selected)
    {
        if (!mr->IsGPUUploaded()) {
            uint32_t key=UploadMesh(mr->GetMeshData());
            mr->SetGPUMeshID(key);
        }
        auto it=m_meshes.find(mr->GetGPUMeshID());
        if (it==m_meshes.end()||!it->second.valid) return;
        const GLMesh& gpu=it->second;
        const Material& mat=mr->GetMaterial();
        Matrix4 model=mr->GetOwner()->GetTransform()->GetWorldMatrix();

        // Draw outline for selected object
        if (selected) {
            glUseProgram(m_outline);
            SetMat4(m_outline,"u_model",model);
            SetMat4(m_outline,"u_view",view);
            SetMat4(m_outline,"u_proj",proj);
            glUniform1f(glGetUniformLocation(m_outline,"u_outlineSize"),0.03f);
            glUniform4f(glGetUniformLocation(m_outline,"u_color"),
                1.0f,0.8f,0.0f,1.0f);
            glCullFace(GL_FRONT);
            glBindVertexArray(gpu.vao);
            glDrawElements(GL_TRIANGLES,gpu.indexCount,GL_UNSIGNED_INT,nullptr);
            glCullFace(GL_BACK);
        }

        // Draw mesh with Phong
        glUseProgram(m_phong);
        SetMat4(m_phong,"u_model",model);
        SetMat4(m_phong,"u_view",view);
        SetMat4(m_phong,"u_proj",proj);

        // Normal matrix
        float nm[9]; BuildNormalMatrix(model,nm);
        glUniformMatrix3fv(glGetUniformLocation(m_phong,"u_normalMatrix"),
            1,GL_FALSE,nm);

        // Material
        glUniform3f(glGetUniformLocation(m_phong,"u_albedo"),
            mat.albedo.r,mat.albedo.g,mat.albedo.b);
        glUniform3f(glGetUniformLocation(m_phong,"u_specular"),
            mat.specular.r,mat.specular.g,mat.specular.b);
        glUniform1f(glGetUniformLocation(m_phong,"u_shininess"),mat.shininess);
        glUniform3f(glGetUniformLocation(m_phong,"u_cameraPos"),
            camPos.x,camPos.y,camPos.z);
        glUniform3f(glGetUniformLocation(m_phong,"u_ambientColor"),
            m_ambient.x,m_ambient.y,m_ambient.z);

        // Light
        Vector3 lDir={0,-1,0},lCol={1,1,1}; float lInt=1.0f;
        for (Light* l:m_lights)
            if (l->GetType()==LightType::Directional) {
                lDir=l->GetDirection();
                lCol=l->GetEffectiveColor();
                lInt=l->GetIntensity(); break;
            }
        glUniform3f(glGetUniformLocation(m_phong,"u_lightDir"),
            lDir.x,lDir.y,lDir.z);
        glUniform3f(glGetUniformLocation(m_phong,"u_lightColor"),
            lCol.x,lCol.y,lCol.z);
        glUniform1f(glGetUniformLocation(m_phong,"u_lightIntensity"),lInt);

        glBindVertexArray(gpu.vao);
        glDrawElements(GL_TRIANGLES,gpu.indexCount,GL_UNSIGNED_INT,nullptr);
        glBindVertexArray(0);
    }

    void BuildGrid(int half, float spacing) {
        std::vector<float> verts;
        float ext=half*spacing;
        for (int i=-half;i<=half;++i){
            float z=i*spacing;
            verts.insert(verts.end(),{-ext,0,z,ext,0,z});
        }
        for (int i=-half;i<=half;++i){
            float x=i*spacing;
            verts.insert(verts.end(),{x,0,-ext,x,0,ext});
        }
        m_gridVertCount=(GLsizei)(verts.size()/3);
        glGenVertexArrays(1,&m_gridVAO);
        glGenBuffers(1,&m_gridVBO);
        glBindVertexArray(m_gridVAO);
        glBindBuffer(GL_ARRAY_BUFFER,m_gridVBO);
        glBufferData(GL_ARRAY_BUFFER,
            verts.size()*sizeof(float),verts.data(),GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),nullptr);
        glBindVertexArray(0);
    }

    void DrawGrid(const Matrix4& view,const Matrix4& proj) {
        if (!m_unlit||!m_gridVAO) return;
        glUseProgram(m_unlit);
        Matrix4 mvp=proj*view;
        glUniformMatrix4fv(glGetUniformLocation(m_unlit,"u_mvp"),
            1,GL_FALSE,mvp.m);
        // Main grid color
        glUniform4f(glGetUniformLocation(m_unlit,"u_color"),
            0.3f,0.3f,0.35f,1.0f);
        glBindVertexArray(m_gridVAO);
        glDrawArrays(GL_LINES,0,m_gridVertCount);
        glBindVertexArray(0);
    }

    GLuint CompileProgram(const std::string& name,
                          const char* vs, const char* fs) {
        GLuint v=glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(v,1,&vs,nullptr); glCompileShader(v);
        if (!CheckShader(v,name+":vert")) { glDeleteShader(v); return 0; }

        GLuint f=glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(f,1,&fs,nullptr); glCompileShader(f);
        if (!CheckShader(f,name+":frag")) {
            glDeleteShader(v); glDeleteShader(f); return 0; }

        GLuint p=glCreateProgram();
        glAttachShader(p,v); glAttachShader(p,f);
        glLinkProgram(p);
        glDeleteShader(v); glDeleteShader(f);

        GLint ok; glGetProgramiv(p,GL_LINK_STATUS,&ok);
        if (!ok) {
            char log[512]; glGetProgramInfoLog(p,512,nullptr,log);
            NOVA_LOG_ERROR("GLRenderer","Link ["+name+"]: "+log);
            glDeleteProgram(p); return 0;
        }
        NOVA_LOG_INFO("GLRenderer","Shader OK: "+name);
        return p;
    }

    bool CheckShader(GLuint id,const std::string& lbl) {
        GLint ok; glGetShaderiv(id,GL_COMPILE_STATUS,&ok);
        if (!ok) {
            char log[512]; glGetShaderInfoLog(id,512,nullptr,log);
            NOVA_LOG_ERROR("GLRenderer","Compile ["+lbl+"]: "+log);
        }
        return ok;
    }

    void SetMat4(GLuint prog,const char* n,const Matrix4& m) {
        glUniformMatrix4fv(glGetUniformLocation(prog,n),1,GL_FALSE,m.m);
    }

    void BuildNormalMatrix(const Matrix4& model,float* o) {
        Matrix4 inv=model.Inverse();
        o[0]=inv.At(0,0);o[1]=inv.At(0,1);o[2]=inv.At(0,2);
        o[3]=inv.At(1,0);o[4]=inv.At(1,1);o[5]=inv.At(1,2);
        o[6]=inv.At(2,0);o[7]=inv.At(2,1);o[8]=inv.At(2,2);
    }

    Camera*    m_camera;
    GameObject* m_selected;
    Vector3    m_ambient;
    int        m_vpW,m_vpH;
    uint32_t   m_nextKey;
    bool       m_showGrid;
    GLuint     m_phong,m_unlit,m_outline;
    GLuint     m_gridVAO,m_gridVBO;
    GLsizei    m_gridVertCount;
    std::unordered_map<uint32_t,GLMesh> m_meshes;
    std::vector<Light*>   m_lights;
    std::vector<RenderCmd> m_cmds;
};

} // namespace Nova
