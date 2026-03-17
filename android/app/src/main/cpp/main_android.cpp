// ============================================================
//  main_android.cpp  —  Nexus Engine 2.0 for Android
// ============================================================

// MUST be first — overrides Logger before any engine header

#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include <android/input.h>
#include <cmath>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <memory>
#include <time.h>

#define LOG_TAG "NexusEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ---- ImGui ----
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_android.h"
#include "imgui/backends/imgui_impl_opengl3.h"

// ---- Engine (Logger already overridden above) ----
#include "Engine/Math/Vector3.h"
#include "Engine/Math/Matrix4.h"
#include "Engine/Core/GameObject.h"
#include "Engine/Core/Transform.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Renderer/Camera.h"
#include "Engine/Renderer/Light.h"
#include "Engine/Renderer/Mesh.h"
#include "Engine/Renderer/MeshRenderer.h"
#include "Engine/Renderer/Material.h"

using namespace Nova;

// ============================================================
//  Shaders
// ============================================================
static const char* PHONG_VERT = R"(#version 300 es
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

static const char* PHONG_FRAG = R"(#version 300 es
precision mediump float;
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

static const char* UNLIT_VERT = R"(#version 300 es
layout(location = 0) in vec3 a_position;
uniform mat4 u_mvp;
void main() { gl_Position = u_mvp * vec4(a_position, 1.0); }
)";

static const char* UNLIT_FRAG = R"(#version 300 es
precision mediump float;
uniform vec4 u_color;
out vec4 fragColor;
void main() { fragColor = u_color; }
)";

// ============================================================
//  GPU Mesh
// ============================================================
struct GLMesh {
    GLuint vao=0, vbo=0, ebo=0;
    GLsizei indexCount=0;
    bool valid=false;
};

static GLuint g_phong=0, g_unlit=0;
static GLuint g_gridVAO=0, g_gridVBO=0;
static GLsizei g_gridVertCount=0;
static std::unordered_map<uint32_t,GLMesh> g_meshes;
static uint32_t g_nextMeshKey=1;
static int g_vpW=0, g_vpH=0;
static Vector3 g_ambient={0.08f,0.08f,0.13f};

static EGLDisplay g_eglDisplay = EGL_NO_DISPLAY;
static EGLSurface g_eglSurface = EGL_NO_SURFACE;
static EGLContext g_eglContext  = EGL_NO_CONTEXT;
static bool g_engineReady = false;

static Scene*           g_scene    = nullptr;
static GameObject*      g_selected = nullptr;
static float g_lastTX=0, g_lastTY=0;
static bool  g_viewportDrag=false;
static float g_joyBaseX=0,g_joyBaseY=0;
static float g_joyDX=0,g_joyDY=0;
static bool  g_joyActive=false;

// ============================================================
//  Timing
// ============================================================
static double GetTimeSec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

// ============================================================
//  Shaders
// ============================================================
static GLuint CompileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s,1,&src,nullptr);
    glCompileShader(s);
    GLint ok=0; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
    if (!ok) {
        char log[512]={}; glGetShaderInfoLog(s,512,nullptr,log);
        LOGE("Shader error: %s", log);
        glDeleteShader(s); return 0;
    }
    return s;
}

static GLuint CompileProgram(const char* vs, const char* fs) {
    GLuint v=CompileShader(GL_VERTEX_SHADER,vs);
    GLuint f=CompileShader(GL_FRAGMENT_SHADER,fs);
    if (!v||!f) { glDeleteShader(v); glDeleteShader(f); return 0; }
    GLuint p=glCreateProgram();
    glAttachShader(p,v); glAttachShader(p,f);
    glLinkProgram(p);
    glDeleteShader(v); glDeleteShader(f);
    GLint ok=0; glGetProgramiv(p,GL_LINK_STATUS,&ok);
    if (!ok) {
        char log[512]={}; glGetProgramInfoLog(p,512,nullptr,log);
        LOGE("Link error: %s", log);
        glDeleteProgram(p); return 0;
    }
    return p;
}

static void SetMat4(GLuint prog, const char* name, const Matrix4& m) {
    glUniformMatrix4fv(glGetUniformLocation(prog,name),1,GL_FALSE,m.m);
}

static void BuildNormalMatrix(const Matrix4& model, float* o) {
    Matrix4 inv=model.Inverse();
    o[0]=inv.At(0,0);o[1]=inv.At(0,1);o[2]=inv.At(0,2);
    o[3]=inv.At(1,0);o[4]=inv.At(1,1);o[5]=inv.At(1,2);
    o[6]=inv.At(2,0);o[7]=inv.At(2,1);o[8]=inv.At(2,2);
}

static uint32_t UploadMesh(const MeshData& data) {
    GLMesh gpu;
    glGenVertexArrays(1,&gpu.vao);
    glGenBuffers(1,&gpu.vbo);
    glGenBuffers(1,&gpu.ebo);
    glBindVertexArray(gpu.vao);
    glBindBuffer(GL_ARRAY_BUFFER,gpu.vbo);
    glBufferData(GL_ARRAY_BUFFER,data.vertices.size()*sizeof(Vertex),data.vertices.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,gpu.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,data.indices.size()*sizeof(uint32_t),data.indices.data(),GL_STATIC_DRAW);
    int stride=sizeof(Vertex);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,stride,(void*)offsetof(Vertex,position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,stride,(void*)offsetof(Vertex,normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,stride,(void*)offsetof(Vertex,uv));
    glBindVertexArray(0);
    gpu.indexCount=(GLsizei)data.indices.size();
    gpu.valid=true;
    uint32_t key=g_nextMeshKey++;
    g_meshes[key]=gpu;
    return key;
}

static void BuildGrid() {
    int half=20;
    std::vector<float> verts;
    float ext=(float)half;
    for(int i=-half;i<=half;++i){float z=(float)i;verts.insert(verts.end(),{-ext,0,z,ext,0,z});}
    for(int i=-half;i<=half;++i){float x=(float)i;verts.insert(verts.end(),{x,0,-ext,x,0,ext});}
    g_gridVertCount=(GLsizei)(verts.size()/3);
    glGenVertexArrays(1,&g_gridVAO);
    glGenBuffers(1,&g_gridVBO);
    glBindVertexArray(g_gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER,g_gridVBO);
    glBufferData(GL_ARRAY_BUFFER,verts.size()*sizeof(float),verts.data(),GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),nullptr);
    glBindVertexArray(0);
}

static bool InitRenderer(int w, int h) {
    g_vpW=w; g_vpH=h;
    glViewport(0,0,w,h);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    g_phong=CompileProgram(PHONG_VERT,PHONG_FRAG);
    g_unlit=CompileProgram(UNLIT_VERT,UNLIT_FRAG);
    if(!g_phong||!g_unlit){ LOGE("Shader compile failed!"); return false; }
    BuildGrid();
    LOGI("Renderer OK %dx%d", w, h);
    return true;
}

// ============================================================
//  Editor Camera
// ============================================================
class EditorCamAndroid {
public:
    GameObject* go=nullptr;
    Camera*     cam=nullptr;
    float yaw=-90.f, pitch=-20.f;
    float moveSpeed=5.f, lookSens=0.25f;
    bool moveF=false,moveB=false,moveL=false,moveR=false;
    bool moveU=false,moveD=false;
    bool lookL=false,lookR=false,lookU=false,lookD=false;

    void Init(Scene* scene, float aspect) {
        go  = scene->CreateGameObject("EditorCamera");
        cam = go->AddComponent<Camera>();
        cam->SetFOV(60.f); cam->SetAspect(aspect);
        cam->SetNear(0.05f); cam->SetFar(500.f);
        go->GetTransform()->SetPosition({0,4,10});
        ApplyRot();
    }

    float joyX=0,joyY=0;
    void Update(float dt) {
        if (!go||!cam) return;
        float spd=moveSpeed*dt;
        Vector3 fwd=GetFwd(), right=GetRight();
        Vector3 pos=go->GetTransform()->GetPosition();
        float deadzone=0.1f;
        float inputMagnitude=sqrtf(joyX*joyX+joyY*joyY);
        if(inputMagnitude>deadzone){
            Vector3 camFwd=GetFwd();
            camFwd.y=0.f;
            float fwdLen=sqrtf(camFwd.x*camFwd.x+camFwd.z*camFwd.z);
            if(fwdLen<0.001f){camFwd={0.f,0.f,-1.f};}
            else{camFwd.x/=fwdLen;camFwd.z/=fwdLen;}
            // Derive right from yaw directly - avoids cross product sign issues
            const float D2R=3.14159265f/180.f;
            float rightYaw=(g_edCam.yaw+90.f)*D2R;
            Vector3 camRight={std::cos(rightYaw),0.f,std::sin(rightYaw)};
            // joyX = right, joyY = down on screen
            // joystick up = -joyY = forward, joystick right = joyX = right
            float moveForward=-joyY;
            float moveRight=-joyX;
            float rawInputLen=sqrtf(moveForward*moveForward+moveRight*moveRight);
            if(rawInputLen>1.f){moveForward/=rawInputLen;moveRight/=rawInputLen;}
            pos.x+=(camRight.x*moveForward+camFwd.x*moveRight)*spd*3.f;
            pos.z+=(camRight.z*moveForward+camFwd.z*moveRight)*spd*3.f;
        }
        if(moveF) pos=pos+fwd*spd;
        if(moveB) pos=pos-fwd*spd;
        if(moveL) pos=pos-right*spd;
        if(moveR) pos=pos+right*spd;
        if(moveU) pos.y+=spd;
        if(moveD) pos.y-=spd;
        go->GetTransform()->SetPosition(pos);
        float ls=60.f*dt;
        if(lookL){yaw-=ls;ApplyRot();}
        if(lookR){yaw+=ls;ApplyRot();}
        if(lookU){pitch=std::min(pitch+ls,89.f);ApplyRot();}
        if(lookD){pitch=std::max(pitch-ls,-89.f);ApplyRot();}
        moveF=moveB=moveL=moveR=moveU=moveD=false;
        joyX=0;joyY=0;
        lookL=lookR=lookU=lookD=false;
    }

    void OnTouchDrag(float dx, float dy) {
        yaw-=dx*lookSens; pitch-=dy*lookSens;
        pitch=std::max(-89.f,std::min(89.f,pitch));
        ApplyRot();
    }

    void SetAspect(float a){ if(cam) cam->SetAspect(a); }

    Vector3 GetFwd() const {
        const float D2R=3.14159265f/180.f;
        float yr=yaw*D2R, pr=pitch*D2R;
        return Vector3(std::cos(pr)*std::cos(yr),std::sin(pr),std::cos(pr)*std::sin(yr)).Normalized();
    }
    Vector3 GetRight() const { return GetFwd().Cross({0,1,0}).Normalized(); }

private:
    void ApplyRot(){ if(go) go->GetTransform()->SetEulerAngles({pitch,yaw,0}); }
};

static EditorCamAndroid g_edCam;

// ============================================================
//  Scene Rendering
// ============================================================
static void RenderScene(Scene* scene, EditorCamAndroid& edCam, GameObject* selected) {
    if (!scene||!edCam.cam) return;
    Matrix4 view=edCam.cam->GetViewMatrix();
    Matrix4 proj=edCam.cam->GetProjectionMatrix();
    Vector3 camPos=edCam.cam->GetPosition();

    // Primary light
    Vector3 lDir={0,-1,0},lCol={1,1,1}; float lInt=1.f;
    scene->ForEach([&](GameObject* go){
        if (!go->IsActive()) return;
        Light* l=go->GetComponent<Light>();
        if (l&&l->IsEnabled()&&l->GetType()==LightType::Directional) {
            lDir=l->GetDirection();
            Vector3 ec=l->GetEffectiveColor();
            lCol={ec.x,ec.y,ec.z};
            lInt=l->GetIntensity();
        }
    });

    // Draw meshes
    scene->ForEach([&](GameObject* go){
        if (!go->IsActive()) return;
        MeshRenderer* mr=go->GetComponent<MeshRenderer>();
        if (!mr||!mr->IsEnabled()||!mr->HasMesh()) return;

        if (!mr->IsGPUUploaded()) {
            uint32_t key=UploadMesh(mr->GetMeshData());
            mr->SetGPUMeshID(key);
        }
        auto it=g_meshes.find(mr->GetGPUMeshID());
        if (it==g_meshes.end()||!it->second.valid) return;
        const GLMesh& gpu=it->second;
        const Material& mat=mr->GetMaterial();
        Matrix4 model=go->GetTransform()->GetWorldMatrix();

        glUseProgram(g_phong);
        SetMat4(g_phong,"u_model",model);
        SetMat4(g_phong,"u_view",view);
        SetMat4(g_phong,"u_proj",proj);
        float nm[9]; BuildNormalMatrix(model,nm);
        glUniformMatrix3fv(glGetUniformLocation(g_phong,"u_normalMatrix"),1,GL_FALSE,nm);
        glUniform3f(glGetUniformLocation(g_phong,"u_albedo"),mat.albedo.r,mat.albedo.g,mat.albedo.b);
        glUniform3f(glGetUniformLocation(g_phong,"u_specular"),mat.specular.r,mat.specular.g,mat.specular.b);
        glUniform1f(glGetUniformLocation(g_phong,"u_shininess"),mat.shininess);
        glUniform3f(glGetUniformLocation(g_phong,"u_cameraPos"),camPos.x,camPos.y,camPos.z);
        glUniform3f(glGetUniformLocation(g_phong,"u_ambientColor"),g_ambient.x,g_ambient.y,g_ambient.z);
        glUniform3f(glGetUniformLocation(g_phong,"u_lightDir"),lDir.x,lDir.y,lDir.z);
        glUniform3f(glGetUniformLocation(g_phong,"u_lightColor"),lCol.x,lCol.y,lCol.z);
        glUniform1f(glGetUniformLocation(g_phong,"u_lightIntensity"),lInt);

        glBindVertexArray(gpu.vao);
        glDrawElements(GL_TRIANGLES,gpu.indexCount,GL_UNSIGNED_INT,nullptr);
        glBindVertexArray(0);
    });

    // Grid
    if (g_unlit&&g_gridVAO) {
        glUseProgram(g_unlit);
        Matrix4 mvp=proj*view;
        glUniformMatrix4fv(glGetUniformLocation(g_unlit,"u_mvp"),1,GL_FALSE,mvp.m);
        glUniform4f(glGetUniformLocation(g_unlit,"u_color"),0.3f,0.3f,0.35f,1.f);
        glBindVertexArray(g_gridVAO);
        glDrawArrays(GL_LINES,0,g_gridVertCount);
        glBindVertexArray(0);
    }
}

// ============================================================
//  Demo Scene
// ============================================================
static void BuildDemoScene(Scene* scene) {
    GameObject* sun=scene->CreateGameObject("Sun");
    sun->GetTransform()->SetEulerAngles({50,-40,0});
    Light* sl=sun->AddComponent<Light>();
    sl->SetType(LightType::Directional);
    sl->SetColor(Color(1.f,0.92f,0.80f,1.f)); sl->SetIntensity(1.1f);

    GameObject* fill=scene->CreateGameObject("FillLight");
    fill->GetTransform()->SetEulerAngles({20,130,0});
    Light* fl=fill->AddComponent<Light>();
    fl->SetType(LightType::Directional);
    fl->SetColor(Color(0.4f,0.5f,0.8f,1.f)); fl->SetIntensity(0.4f);

    GameObject* ground=scene->CreateGameObject("Ground");
    ground->GetTransform()->SetScale({12,1,12});
    MeshRenderer* gMR=ground->AddComponent<MeshRenderer>();
    gMR->SetMeshData(PrimitiveMesh::CreatePlane(1,1,8,8));
    Material gMat=Material::Colored({0.20f,0.38f,0.20f}); gMat.name="Ground";
    gMR->SetMaterial(gMat);

    GameObject* cube=scene->CreateGameObject("Cube");
    cube->GetTransform()->SetPosition({0,0.5f,0});
    MeshRenderer* cMR=cube->AddComponent<MeshRenderer>();
    cMR->SetMeshData(PrimitiveMesh::CreateCube(1.f));
    Material cMat=Material::Colored({0.85f,0.22f,0.18f});
    cMat.specular={0.8f,0.8f,0.8f}; cMat.shininess=64.f; cMat.name="Red";
    cMR->SetMaterial(cMat);

    GameObject* sph=scene->CreateGameObject("Sphere");
    sph->GetTransform()->SetPosition({-2.5f,0.6f,0});
    MeshRenderer* sMR=sph->AddComponent<MeshRenderer>();
    sMR->SetMeshData(PrimitiveMesh::CreateSphere(0.6f,24,24));
    Material sMat=Material::Colored({0.18f,0.48f,1.f});
    sMat.shininess=80.f; sMat.name="Blue";
    sMR->SetMaterial(sMat);

    GameObject* cyl=scene->CreateGameObject("Cylinder");
    cyl->GetTransform()->SetPosition({2.5f,1.f,0});
    MeshRenderer* yMR=cyl->AddComponent<MeshRenderer>();
    yMR->SetMeshData(PrimitiveMesh::CreateCylinder(0.5f,2.f,24));
    Material yMat=Material::Colored({0.95f,0.78f,0.1f});
    yMat.shininess=32.f; yMat.name="Yellow";
    yMR->SetMaterial(yMat);

    GameObject* gs=scene->CreateGameObject("GreenSphere");
    gs->GetTransform()->SetPosition({0,0.7f,-2.5f});
    MeshRenderer* gsMR=gs->AddComponent<MeshRenderer>();
    gsMR->SetMeshData(PrimitiveMesh::CreateSphere(0.7f,20,20));
    Material gsMat=Material::Colored({0.18f,0.82f,0.28f});
    gsMat.shininess=48.f; gsMat.name="Green";
    gsMR->SetMaterial(gsMat);
}

// ============================================================
//  Editor UI
// ============================================================
static void DrawEditorUI(Scene* scene, EditorCamAndroid& edCam,
                          GameObject*& selected, int w, int h) {
    float fps=ImGui::GetIO().Framerate;

    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImVec2((float)w,50));
    ImGui::Begin("##title",nullptr,
        ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|
        ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar|
        ImGuiWindowFlags_NoBackground);
    ImGui::TextColored(ImVec4(0.2f,0.6f,1.f,1.f),"NEXUS ENGINE 2.0");
    ImGui::SameLine(0,20);
    if(ImGui::BeginMenu("File")) {
        if(ImGui::MenuItem("New Scene")) { }
        if(ImGui::MenuItem("Add Cube")) {
            GameObject* nb=scene->CreateGameObject("Cube");
            nb->GetTransform()->SetPosition({0,0.5f,0});
            MeshRenderer* mr=nb->AddComponent<MeshRenderer>();
            mr->SetMeshData(PrimitiveMesh::CreateCube(1.f));
            mr->SetMaterial(Material::Colored({0.6f,0.6f,0.6f}));
        }
        if(ImGui::MenuItem("Add Sphere")) {
            GameObject* nb=scene->CreateGameObject("Sphere");
            nb->GetTransform()->SetPosition({0,0.5f,0});
            MeshRenderer* mr=nb->AddComponent<MeshRenderer>();
            mr->SetMeshData(PrimitiveMesh::CreateSphere(0.5f,24,24));
            mr->SetMaterial(Material::Colored({0.3f,0.5f,1.f}));
        }
        ImGui::EndMenu();
    }
    ImGui::SameLine(0,10);
    if(ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Hierarchy", nullptr, nullptr, false);
        ImGui::MenuItem("Inspector", nullptr, nullptr, false);
        ImGui::EndMenu();
    }
    ImGui::SameLine(0,30);
    ImGui::Text("FPS: %.0f jX:%.2f jY:%.2f", fps, g_joyDX, g_joyDY);
    // Direction debug
    Vector3 fwd=g_edCam.GetFwd();
    const char* dir="?";
    if(fwd.z<-0.5f) dir="FORWARD(-Z)";
    else if(fwd.z>0.5f) dir="BACKWARD(+Z)";
    else if(fwd.x>0.5f) dir="RIGHT(+X)";
    else if(fwd.x<-0.5f) dir="LEFT(-X)";
    ImGui::SameLine();
    ImGui::Text("Dir:%s fwd(%.1f,%.1f)", dir, fwd.x, fwd.z);
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(0,50));
    ImGui::SetNextWindowSize(ImVec2(180,(float)h*0.5f));
    ImGui::Begin("Hierarchy",nullptr,ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize);
    int goIdx=0;
    scene->ForEach([&](GameObject* go){
        if (go->GetName()=="EditorCamera") return;
        ImGui::PushID(goIdx++);
        bool sel=(go==selected);
        if (sel) ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(1.f,0.8f,0.2f,1.f));
        if (ImGui::Button(go->GetName().c_str(),ImVec2(-1,40))) selected=go;
        if (sel) ImGui::PopStyleColor();
        ImGui::PopID();
    });
    if (ImGui::Button("+ Cube",ImVec2(-1,40))) {
        GameObject* nb=scene->CreateGameObject("NewCube");
        nb->GetTransform()->SetPosition({0,0.5f,0});
        MeshRenderer* mr=nb->AddComponent<MeshRenderer>();
        mr->SetMeshData(PrimitiveMesh::CreateCube(1.f));
        mr->SetMaterial(Material::Colored({0.6f,0.6f,0.6f}));
        selected=nb;
    }
    if (selected&&ImGui::Button("Delete",ImVec2(-1,40))) {
        scene->DestroyGameObject(selected); selected=nullptr;
    }
    ImGui::End();

    if (selected) {
        ImGui::SetNextWindowPos(ImVec2((float)w-190,50));
        ImGui::SetNextWindowSize(ImVec2(190,(float)h*0.5f));
        ImGui::Begin("Inspector",nullptr,ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize);
        ImGui::Text("%s", selected->GetName().c_str());
        ImGui::Separator();
        Transform* tr=selected->GetTransform();
        Vector3 pos=tr->GetPosition();
        Vector3 scl=tr->GetScale();
        ImGui::Text("Position");
        if(ImGui::Button("+X",ImVec2(40,35))) pos.x+=0.5f; ImGui::SameLine();
        if(ImGui::Button("-X",ImVec2(40,35))) pos.x-=0.5f; ImGui::SameLine();
        if(ImGui::Button("+Y",ImVec2(40,35))) pos.y+=0.5f; ImGui::SameLine();
        if(ImGui::Button("-Y",ImVec2(40,35))) pos.y-=0.5f;
        if(ImGui::Button("+Z",ImVec2(40,35))) pos.z+=0.5f; ImGui::SameLine();
        if(ImGui::Button("-Z",ImVec2(40,35))) pos.z-=0.5f;
        ImGui::Text("%.1f %.1f %.1f",pos.x,pos.y,pos.z);
        tr->SetPosition(pos);
        ImGui::Separator();
        ImGui::Text("Scale");
        if(ImGui::Button("2x",ImVec2(55,35))) scl=scl*2.f; ImGui::SameLine();
        if(ImGui::Button(".5x",ImVec2(55,35))) scl=scl*0.5f; ImGui::SameLine();
        if(ImGui::Button("Rst",ImVec2(55,35))) scl={1,1,1};
        tr->SetScale(scl);
        ImGui::End();
    }

    // Joystick - drawn with ImDrawList
    float joySize=200.f;
    ImGui::SetNextWindowPos(ImVec2(10,(float)h-joySize-10));
    ImGui::SetNextWindowSize(ImVec2(joySize,joySize));
    ImGui::SetNextWindowBgAlpha(0.3f);
    ImGui::Begin("##joy",nullptr,
        ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|
        ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar|
        ImGuiWindowFlags_NoInputs);
    ImDrawList* dl=ImGui::GetWindowDrawList();
    ImVec2 wp=ImGui::GetWindowPos();
    float cx=wp.x+joySize*0.5f,cy=wp.y+joySize*0.5f;
    float outerR=joySize*0.44f,innerR=joySize*0.17f;
    dl->AddCircle(ImVec2(cx,cy),outerR,IM_COL32(80,140,255,150),32,2.5f);
    float kx=cx,ky=cy;
    if(g_joyActive){
        float len=sqrtf(g_joyDX*g_joyDX+g_joyDY*g_joyDY);
        float clamp=std::min(len,outerR);
        if(len>0){kx+=g_joyDX/len*clamp;ky+=g_joyDY/len*clamp;}
    }
    dl->AddCircleFilled(ImVec2(kx,ky),innerR,IM_COL32(100,180,255,220));
    ImGui::End();
    // Apply joystick to camera movement
    if(g_joyActive){
        float len=sqrtf(g_joyDX*g_joyDX+g_joyDY*g_joyDY);
        if(len>12.f){
            float nx=g_joyDX/len,ny=g_joyDY/len;
            edCam.joyX=std::max(-1.f,std::min(1.f,g_joyDX/100.f));
            edCam.joyY=std::max(-1.f,std::min(1.f,g_joyDY/100.f));
        }
    }

}

// ============================================================
//  EGL
// ============================================================
static bool InitEGL(ANativeWindow* window) {
    g_eglDisplay=eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_eglDisplay==EGL_NO_DISPLAY) { LOGE("eglGetDisplay failed"); return false; }
    EGLint major=0,minor=0;
    if (!eglInitialize(g_eglDisplay,&major,&minor)) { LOGE("eglInitialize failed"); return false; }
    LOGI("EGL %d.%d",major,minor);
    const EGLint attribs[]={
        EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE,EGL_WINDOW_BIT,
        EGL_BLUE_SIZE,8,EGL_GREEN_SIZE,8,EGL_RED_SIZE,8,EGL_DEPTH_SIZE,16,
        EGL_NONE
    };
    EGLConfig cfg; EGLint n=0;
    if (!eglChooseConfig(g_eglDisplay,attribs,&cfg,1,&n)||n==0) { LOGE("eglChooseConfig failed"); return false; }
    const EGLint ctxAttr[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    g_eglContext=eglCreateContext(g_eglDisplay,cfg,EGL_NO_CONTEXT,ctxAttr);
    if (g_eglContext==EGL_NO_CONTEXT) { LOGE("eglCreateContext failed"); return false; }
    g_eglSurface=eglCreateWindowSurface(g_eglDisplay,cfg,(EGLNativeWindowType)window,nullptr);
    if (g_eglSurface==EGL_NO_SURFACE) { LOGE("eglCreateWindowSurface failed"); return false; }
    if (!eglMakeCurrent(g_eglDisplay,g_eglSurface,g_eglSurface,g_eglContext)) { LOGE("eglMakeCurrent failed"); return false; }
    eglQuerySurface(g_eglDisplay,g_eglSurface,EGL_WIDTH,&g_vpW);
    eglQuerySurface(g_eglDisplay,g_eglSurface,EGL_HEIGHT,&g_vpH);
    LOGI("Surface %dx%d",g_vpW,g_vpH);
    return true;
}

// ============================================================
//  Engine Init / Shutdown
// ============================================================
static void InitEngine(ANativeWindow* window) {
    LOGI("InitEngine start");
    if (!InitEGL(window)) { LOGE("EGL failed"); return; }
    // Render a frame immediately so Android doesn't kill us
    glClearColor(0.12f,0.12f,0.18f,1.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    eglSwapBuffers(g_eglDisplay,g_eglSurface);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io=ImGui::GetIO();
    io.DisplaySize=ImVec2((float)g_vpW,(float)g_vpH);
    io.FontGlobalScale=1.6f;
    ImGui::StyleColorsDark();
    ImGui_ImplAndroid_Init(window);
    ImGui_ImplOpenGL3_Init("#version 300 es");
    LOGI("ImGui OK");
    if (!InitRenderer(g_vpW,g_vpH)) { LOGE("Renderer failed"); return; }
    g_scene=new Scene();
    BuildDemoScene(g_scene);
    LOGI("Scene built");
    g_edCam.Init(g_scene,(float)g_vpW/(float)g_vpH);
    g_engineReady=true;
    LOGI("Engine ready!");
}

static void ShutdownEngine() {
    if (!g_engineReady) return;
    g_engineReady=false;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplAndroid_Shutdown();
    ImGui::DestroyContext();
    delete g_scene; g_scene=nullptr;
    g_edCam.go=nullptr; g_edCam.cam=nullptr;
    g_selected=nullptr;
    eglMakeCurrent(g_eglDisplay,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
    eglDestroyContext(g_eglDisplay,g_eglContext);
    eglDestroySurface(g_eglDisplay,g_eglSurface);
    eglTerminate(g_eglDisplay);
    g_eglDisplay=EGL_NO_DISPLAY;
    g_eglSurface=EGL_NO_SURFACE;
    g_eglContext=EGL_NO_CONTEXT;
}

// ============================================================
//  Render Frame
// ============================================================
static void RenderFrame() {
    static double lastTime=0;
    double now=GetTimeSec();
    float dt=(float)(now-lastTime); lastTime=now;
    if (dt<=0.f||dt>0.1f) dt=0.016f;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    DrawEditorUI(g_scene,g_edCam,g_selected,g_vpW,g_vpH);
    g_edCam.Update(dt);
    g_scene->Update(dt);
    ImGui::Render();

    glViewport(0,0,g_vpW,g_vpH);
    glClearColor(0.12f,0.12f,0.18f,1.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    RenderScene(g_scene,g_edCam,g_selected);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    eglSwapBuffers(g_eglDisplay,g_eglSurface);
}

// ============================================================
//  Input + Commands
// ============================================================
static int32_t HandleInput(android_app* app, AInputEvent* event) {
    if (!g_engineReady) return 0;
    ImGui_ImplAndroid_HandleInputEvent(event);
    if (AInputEvent_getType(event)==AINPUT_EVENT_TYPE_MOTION) {
        int action=AMotionEvent_getAction(event)&AMOTION_EVENT_ACTION_MASK;
        float tx=AMotionEvent_getX(event,0);
        float ty=AMotionEvent_getY(event,0);
        // Joystick zone bottom-left
        bool inJoy=(tx<220.f&&ty>(float)g_vpH-220.f);
        if(action==AMOTION_EVENT_ACTION_DOWN&&inJoy){
            g_joyActive=true;
            float joySize=200.f;
            g_joyBaseX=10.f+joySize*0.5f;
            g_joyBaseY=(float)g_vpH-joySize*0.5f-10.f;
            g_joyDX=0;g_joyDY=0;
        } else if(action==AMOTION_EVENT_ACTION_MOVE&&g_joyActive){
            g_joyDX=tx-g_joyBaseX;g_joyDY=ty-g_joyBaseY;
        } else if(action==AMOTION_EVENT_ACTION_UP&&g_joyActive){
            g_joyActive=false;g_joyDX=0;g_joyDY=0;
        }
        bool inUI=(tx<200&&ty>50&&ty<g_vpH*0.5f+50)||
                  (tx>g_vpW-210&&ty>50&&ty<g_vpH*0.5f+50)||
                  (tx<200&&ty>g_vpH-180)||
                  (tx>g_vpW-210&&ty>g_vpH-180);
        if (action==AMOTION_EVENT_ACTION_DOWN&&!inUI){g_viewportDrag=true;g_lastTX=tx;g_lastTY=ty;}
        else if (action==AMOTION_EVENT_ACTION_MOVE&&g_viewportDrag){g_edCam.OnTouchDrag(tx-g_lastTX,ty-g_lastTY);g_lastTX=tx;g_lastTY=ty;}
        else if (action==AMOTION_EVENT_ACTION_UP) g_viewportDrag=false;
    }
    return 0;
}

static void HandleCmd(android_app* app, int32_t cmd) {
    switch(cmd) {
        case APP_CMD_INIT_WINDOW:   if(app->window) InitEngine(app->window); break;
        case APP_CMD_TERM_WINDOW:   ShutdownEngine(); break;
        case APP_CMD_WINDOW_RESIZED:
            if (g_engineReady) {
                eglQuerySurface(g_eglDisplay,g_eglSurface,EGL_WIDTH,&g_vpW);
                eglQuerySurface(g_eglDisplay,g_eglSurface,EGL_HEIGHT,&g_vpH);
                ImGui::GetIO().DisplaySize=ImVec2((float)g_vpW,(float)g_vpH);
                glViewport(0,0,g_vpW,g_vpH);
                g_edCam.SetAspect((float)g_vpW/(float)g_vpH);
            }
            break;
        case APP_CMD_LOST_FOCUS: g_viewportDrag=false; break;
        default: break;
    }
}

// ============================================================
//  Entry Point
// ============================================================
void android_main(android_app* app) {
    app->onAppCmd=HandleCmd;
    app->onInputEvent=HandleInput;
    while (true) {
        int events=0;
        android_poll_source* source=nullptr;
        int result=ALooper_pollOnce(g_engineReady?0:-1,nullptr,&events,(void**)&source);
        if (result==ALOOPER_POLL_ERROR) break;
        if (source) source->process(app,source);
        if (app->destroyRequested) break;
        if (g_engineReady) RenderFrame();
    }
    ShutdownEngine();
}
