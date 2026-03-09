#pragma once
// ============================================================
//  EditorUI.h  —  Nexus Engine 2.0 - Mobile-Friendly Editor UI
//  Big buttons, touch-friendly panels, virtual joystick
// ============================================================
#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_glfw.h"
#include "../imgui/backends/imgui_impl_opengl3.h"
#include "../Scene/Scene.h"
#include "../Core/GameObject.h"
#include "../Core/Transform.h"
#include "../Core/Time.h"
#include "../Renderer/MeshRenderer.h"
#include "../Renderer/Camera.h"
#include "../Renderer/Light.h"
#include "../Renderer/Mesh.h"
#include "../Renderer/GLRenderer.h"
#include "EditorCamera.h"
#include <GLFW/glfw3.h>
#include <string>
#include <functional>
#include <cstring>
#include <cmath>

namespace Nova {

class EditorUI {
public:
    EditorUI()
        : m_scene(nullptr), m_selected(nullptr),
          m_editorCam(nullptr),
          m_playing(false), m_showStats(true),
          m_showGrid(true), m_showMovepad(true),
          m_showLookpad(true)
    {}

    bool Init(GLFWwindow* window, Scene* scene, EditorCamera* cam) {
        m_scene     = scene;
        m_editorCam = cam;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.FontGlobalScale = 1.6f; // Large text for mobile

        ApplyMobileTheme();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330 core");
        return true;
    }

    void Shutdown() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void BeginFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void Render() {
        ImGuiIO& io = ImGui::GetIO();
        float sw = io.DisplaySize.x;
        float sh = io.DisplaySize.y;

        DrawMenuBar();
        DrawTitleBar(sw);
        DrawToolbar(sw);
        DrawHierarchy(sh);
        DrawInspector(sw, sh);
        if (m_showStats)   DrawStats();
        if (m_showMovepad) DrawMovePad(sw, sh);
        if (m_showLookpad) DrawLookPad(sw, sh);
    }

    void EndFrame() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    GameObject* GetSelected() const { return m_selected; }
    bool        IsPlaying()   const { return m_playing;  }
    bool        ShowGrid()    const { return m_showGrid; }
    void        SetScene(Scene* s)  { m_scene=s; m_selected=nullptr; }

    std::function<void()> OnPlay;
    std::function<void()> OnStop;

private:
    // ---- Big mobile theme ----
    void ApplyMobileTheme() {
        ImGui::StyleColorsDark();
        ImGuiStyle& s = ImGui::GetStyle();
        s.WindowRounding    = 8.0f;
        s.FrameRounding     = 6.0f;
        s.GrabRounding      = 6.0f;
        s.ScrollbarRounding = 6.0f;
        s.FramePadding      = {8, 6};
        s.ItemSpacing       = {8, 8};
        s.WindowPadding     = {10, 10};
        s.ScrollbarSize     = 20.0f; // Fat scrollbar for fingers
        s.GrabMinSize       = 20.0f;

        auto* c = ImGui::GetStyle().Colors;
        c[ImGuiCol_WindowBg]         = {0.10f,0.10f,0.14f,0.95f};
        c[ImGuiCol_Header]           = {0.20f,0.40f,0.70f,0.7f};
        c[ImGuiCol_HeaderHovered]    = {0.25f,0.50f,0.85f,0.9f};
        c[ImGuiCol_HeaderActive]     = {0.20f,0.45f,0.80f,1.0f};
        c[ImGuiCol_Button]           = {0.18f,0.32f,0.58f,0.9f};
        c[ImGuiCol_ButtonHovered]    = {0.25f,0.45f,0.75f,1.0f};
        c[ImGuiCol_ButtonActive]     = {0.35f,0.55f,0.90f,1.0f};
        c[ImGuiCol_TitleBg]          = {0.08f,0.08f,0.12f,1.0f};
        c[ImGuiCol_TitleBgActive]    = {0.12f,0.20f,0.40f,1.0f};
        c[ImGuiCol_FrameBg]          = {0.16f,0.16f,0.22f,1.0f};
        c[ImGuiCol_CheckMark]        = {0.3f,0.75f,1.0f,1.0f};
        c[ImGuiCol_SliderGrab]       = {0.3f,0.65f,1.0f,1.0f};
        c[ImGuiCol_SliderGrabActive] = {0.4f,0.75f,1.0f,1.0f};
    }

    // ---- Nexus Engine 2.0 Title Bar ----
    void DrawTitleBar(float sw) {
        ImGui::SetNextWindowPos({0, 20});
        ImGui::SetNextWindowSize({sw, 24});
        ImGui::SetNextWindowBgAlpha(0.85f);
        ImGui::Begin("##titlebar", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoBringToFrontOnFocus);
        // Center the title text
        const char* title = "Nexus Engine 2.0";
        float textW = ImGui::CalcTextSize(title).x;
        ImGui::SetCursorPosX((sw - textW) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, {0.2f, 0.6f, 1.0f, 1.0f});
        ImGui::Text("%s", title);
        ImGui::PopStyleColor();
        ImGui::End();
    }

    // ---- New Scene ----
    void NewScene() {
        if (!m_scene) return;
        // Destroy all objects
        std::vector<GameObject*> all = m_scene->GetRootObjects();
        for (auto* go : all)
            m_scene->DestroyGameObjectImmediate(go);
        m_selected = nullptr;
        GLRenderer::Get().SetSelectedObject(nullptr);
    }

    // ---- Menu bar ----
    void DrawMenuBar() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Scene")) NewScene();
                ImGui::Separator();
                if (ImGui::MenuItem("Exit"))
                    glfwSetWindowShouldClose(glfwGetCurrentContext(), GLFW_TRUE);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Create")) {
                if (ImGui::MenuItem("Cube"))     CreateObject("Cube",    "cube");
                if (ImGui::MenuItem("Sphere"))   CreateObject("Sphere",  "sphere");
                if (ImGui::MenuItem("Plane"))    CreateObject("Plane",   "plane");
                if (ImGui::MenuItem("Cylinder")) CreateObject("Cylinder","cylinder");
                if (ImGui::MenuItem("Light"))    CreateObject("Light",   "pointlight");
                if (ImGui::MenuItem("Empty"))    CreateObject("Object",  "empty");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Grid",     nullptr, &m_showGrid);
                ImGui::MenuItem("Stats",    nullptr, &m_showStats);
                ImGui::MenuItem("Move Pad", nullptr, &m_showMovepad);
                ImGui::MenuItem("Look Pad", nullptr, &m_showLookpad);
                GLRenderer::Get().SetShowGrid(m_showGrid);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    // ---- Toolbar ----
    void DrawToolbar(float sw) {
        float toolbarH = 52.0f;
        ImGui::SetNextWindowPos({0, 44});
        ImGui::SetNextWindowSize({sw, toolbarH});
        ImGui::SetNextWindowBgAlpha(0.90f);
        ImGui::Begin("##toolbar", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        float btnH = 36.0f;

        // Play/Stop
        if (!m_playing) {
            ImGui::PushStyleColor(ImGuiCol_Button,        {0.1f,0.55f,0.1f,1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.15f,0.7f,0.15f,1.0f});
            if (ImGui::Button("PLAY", {80,btnH})) {
                m_playing = true;
                if (OnPlay) OnPlay();
            }
            ImGui::PopStyleColor(2);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button,        {0.65f,0.1f,0.1f,1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.8f,0.15f,0.15f,1.0f});
            if (ImGui::Button("STOP", {80,btnH})) {
                m_playing = false;
                if (OnStop) OnStop();
            }
            ImGui::PopStyleColor(2);
        }

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, {0.25f,0.25f,0.35f,1.0f});
        if (ImGui::Button("+Cube",   {72,btnH})) CreateObject("Cube",    "cube");
        ImGui::SameLine();
        if (ImGui::Button("+Sphere", {80,btnH})) CreateObject("Sphere",  "sphere");
        ImGui::SameLine();
        if (ImGui::Button("+Plane",  {76,btnH})) CreateObject("Plane",   "plane");
        ImGui::SameLine();
        if (ImGui::Button("+Light",  {76,btnH})) CreateObject("Light",   "pointlight");
        ImGui::PopStyleColor();

        ImGui::SameLine();
        if (m_selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, {0.55f,0.1f,0.1f,1.0f});
            if (ImGui::Button("DELETE", {80,btnH})) {
                if (m_scene) m_scene->DestroyGameObjectImmediate(m_selected);
                m_selected = nullptr;
                GLRenderer::Get().SetSelectedObject(nullptr);
            }
            ImGui::PopStyleColor();
        }

        ImGui::End();
    }

    // ---- Hierarchy ----
    void DrawHierarchy(float sh) {
        float panelW = 160.0f;
        float panelY = 98.0f;
        float panelH = sh * 0.45f;

        ImGui::SetNextWindowPos({0, panelY});
        ImGui::SetNextWindowSize({panelW, panelH});
        ImGui::Begin("Hierarchy", nullptr,
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

        if (!m_scene) { ImGui::Text("No scene"); ImGui::End(); return; }
        ImGui::TextDisabled("%d objects", m_scene->ObjectCount());
        ImGui::Separator();

        for (auto* go : m_scene->GetRootObjects())
            DrawNode(go);

        ImGui::End();
    }

    void DrawNode(GameObject* go) {
        if (!go) return;
        bool hasChildren = go->GetTransform()->HasChildren();
        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth;
        if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf;
        if (go == m_selected) flags |= ImGuiTreeNodeFlags_Selected;

        if (!go->IsActive())
            ImGui::PushStyleColor(ImGuiCol_Text, {0.5f,0.5f,0.5f,1.0f});

        bool open = ImGui::TreeNodeEx(
            (void*)(intptr_t)go->GetID(), flags,
            "%s", go->GetName().c_str());

        if (!go->IsActive()) ImGui::PopStyleColor();

        if (ImGui::IsItemClicked()) {
            m_selected = go;
            GLRenderer::Get().SetSelectedObject(go);
        }

        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Delete")) {
                m_scene->DestroyGameObjectImmediate(go);
                if (m_selected == go) {
                    m_selected = nullptr;
                    GLRenderer::Get().SetSelectedObject(nullptr);
                }
            }
            ImGui::EndPopup();
        }

        if (open) {
            if (hasChildren)
                for (Transform* c : go->GetTransform()->GetChildren())
                    if (c->GetOwner()) DrawNode(c->GetOwner());
            ImGui::TreePop();
        }
    }

    // ---- Inspector ----
    void DrawInspector(float sw, float sh) {
        float panelW = 180.0f;
        float panelY = 98.0f;
        float panelH = sh * 0.45f;

        ImGui::SetNextWindowPos({sw - panelW, panelY});
        ImGui::SetNextWindowSize({panelW, panelH});
        ImGui::Begin("Inspector", nullptr,
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

        if (!m_selected) {
            ImGui::TextDisabled("Tap an object");
            ImGui::TextDisabled("in Hierarchy");
            ImGui::End(); return;
        }

        // Name
        char buf[128];
        strncpy(buf, m_selected->GetName().c_str(), sizeof(buf));
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##name", buf, sizeof(buf)))
            m_selected->SetName(buf);

        bool active = m_selected->IsActive();
        if (ImGui::Checkbox("Active", &active))
            m_selected->SetActive(active);

        ImGui::Separator();

        // Transform
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            Transform* t = m_selected->GetTransform();

            // Mode buttons: Move / Rotate / Scale
            ImGui::Text("Mode:");
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, m_transformMode==0 ?
                ImVec4{0.2f,0.6f,1.0f,1.0f} : ImVec4{0.2f,0.2f,0.3f,1.0f});
            if (ImGui::Button("Move",   {56,28})) m_transformMode=0;
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, m_transformMode==1 ?
                ImVec4{0.2f,0.6f,1.0f,1.0f} : ImVec4{0.2f,0.2f,0.3f,1.0f});
            if (ImGui::Button("Rotate", {56,28})) m_transformMode=1;
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, m_transformMode==2 ?
                ImVec4{0.2f,0.6f,1.0f,1.0f} : ImVec4{0.2f,0.2f,0.3f,1.0f});
            if (ImGui::Button("Scale",  {56,28})) m_transformMode=2;
            ImGui::PopStyleColor();

            // Snap toggle
            ImGui::Checkbox("Snap", &m_snapEnabled);
            if (m_snapEnabled) {
                ImGui::SameLine();
                ImGui::SetNextItemWidth(80);
                ImGui::DragFloat("##snap", &m_snapValue, 0.05f, 0.01f, 10.0f);
            }
            ImGui::Separator();

            // Position
            Vector3 pos = t->GetPosition();
            float p[3] = {pos.x, pos.y, pos.z};
            ImGui::Text("Position"); ImGui::SetNextItemWidth(-1);
            float dragSpd = m_snapEnabled ? m_snapValue : 0.05f;
            if (ImGui::DragFloat3("##p", p, dragSpd)) {
                if (m_snapEnabled) {
                    p[0] = std::round(p[0]/m_snapValue)*m_snapValue;
                    p[1] = std::round(p[1]/m_snapValue)*m_snapValue;
                    p[2] = std::round(p[2]/m_snapValue)*m_snapValue;
                }
                t->SetPosition({p[0],p[1],p[2]});
            }
            // Step buttons for position
            if (ImGui::Button("-X##px",{30,24})){ pos.x-=dragSpd; t->SetPosition(pos); } ImGui::SameLine();
            if (ImGui::Button("+X##px",{30,24})){ pos.x+=dragSpd; t->SetPosition(pos); } ImGui::SameLine();
            if (ImGui::Button("-Y##py",{30,24})){ pos.y-=dragSpd; t->SetPosition(pos); } ImGui::SameLine();
            if (ImGui::Button("+Y##py",{30,24})){ pos.y+=dragSpd; t->SetPosition(pos); } ImGui::SameLine();
            if (ImGui::Button("-Z##pz",{30,24})){ pos.z-=dragSpd; t->SetPosition(pos); } ImGui::SameLine();
            if (ImGui::Button("+Z##pz",{30,24})){ pos.z+=dragSpd; t->SetPosition(pos); }

            // Rotation
            Vector3 rot = t->GetEulerAngles();
            float r[3] = {rot.x, rot.y, rot.z};
            ImGui::Text("Rotation"); ImGui::SetNextItemWidth(-1);
            if (ImGui::DragFloat3("##r", r, 0.5f))
                t->SetEulerAngles({r[0],r[1],r[2]});
            if (ImGui::Button("-90X",{38,24})){ rot.x-=90; t->SetEulerAngles(rot); } ImGui::SameLine();
            if (ImGui::Button("+90X",{38,24})){ rot.x+=90; t->SetEulerAngles(rot); } ImGui::SameLine();
            if (ImGui::Button("-90Y",{38,24})){ rot.y-=90; t->SetEulerAngles(rot); } ImGui::SameLine();
            if (ImGui::Button("+90Y",{38,24})){ rot.y+=90; t->SetEulerAngles(rot); }

            // Scale
            Vector3 scl = t->GetScale();
            float s[3] = {scl.x, scl.y, scl.z};
            ImGui::Text("Scale"); ImGui::SetNextItemWidth(-1);
            if (ImGui::DragFloat3("##s", s, 0.01f, 0.001f, 100.0f))
                t->SetScale({s[0],s[1],s[2]});
            if (ImGui::Button("0.5x",{38,24})){ t->SetScale(scl*0.5f); } ImGui::SameLine();
            if (ImGui::Button("2x", {30,24})){ t->SetScale(scl*2.0f); } ImGui::SameLine();
            if (ImGui::Button("Reset",{48,24})){ t->SetScale({1,1,1}); }
        }

        // MeshRenderer
        if (auto* mr = m_selected->GetComponent<MeshRenderer>()) {
            if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
                Material& mat = mr->GetMaterial();
                float col[3] = {mat.albedo.r, mat.albedo.g, mat.albedo.b};
                if (ImGui::ColorEdit3("Color", col)) {
                    mat.albedo = {col[0],col[1],col[2]};
                    mr->MarkDirty();
                }
                ImGui::SliderFloat("Shine", &mat.shininess, 1.0f, 256.0f);
            }
        }

        // Light
        if (auto* lt = m_selected->GetComponent<Light>()) {
            if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                const char* types[]={"Directional","Point","Spot","Ambient"};
                int t=(int)lt->GetType();
                if (ImGui::Combo("Type",&t,types,4)) lt->SetType((LightType)t);
                float col[3]={lt->GetColor().r,lt->GetColor().g,lt->GetColor().b};
                if (ImGui::ColorEdit3("Color",col)) lt->SetColor({col[0],col[1],col[2]});
                float intensity=lt->GetIntensity();
                if (ImGui::SliderFloat("Intensity",&intensity,0.0f,5.0f))
                    lt->SetIntensity(intensity);
            }
        }

        // Add component
        ImGui::Separator();
        if (!m_selected->HasComponent<MeshRenderer>()) {
            if (ImGui::Button("+ MeshRenderer", {-1,0}))
                m_selected->AddComponent<MeshRenderer>();
        }
        if (!m_selected->HasComponent<Light>()) {
            if (ImGui::Button("+ Light", {-1,0})) {
                auto* l = m_selected->AddComponent<Light>();
                l->SetType(LightType::Point);
            }
        }

        ImGui::End();
    }

    // ---- Stats overlay ----
    void DrawStats() {
        ImGui::SetNextWindowPos({205, 102});
        ImGui::SetNextWindowBgAlpha(0.60f);
        ImGui::SetNextWindowSize({155, 80});
        ImGui::Begin("##stats", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::Text("FPS:  %.0f", Time::Get().FPS());
        ImGui::Text("Objs: %d",   m_scene ? m_scene->ObjectCount() : 0);
        ImGui::End();
    }

    // ---- Virtual Move Pad (bottom left) ----
    void DrawMovePad(float sw, float sh) {
        if (!m_editorCam) return;

        float padW = 180.0f;
        float padH = 195.0f;
        float padX = 5.0f;
        float padY = sh - padH - 5.0f;

        ImGui::SetNextWindowPos({padX, padY});
        ImGui::SetNextWindowSize({padW, padH});
        ImGui::SetNextWindowBgAlpha(0.75f);
        ImGui::Begin("##movepad", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImGui::TextDisabled("  MOVE");

        float btnSz = 52.0f;
        float spacing = 4.0f;

        // Row 1: Up arrow (forward)
        ImGui::SetCursorPosX((padW - btnSz) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Button,        {0.15f,0.35f,0.65f,0.95f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.25f,0.48f,0.80f,1.00f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {0.40f,0.62f,1.00f,1.00f});

        // Forward button
        ImGui::SetCursorPosX((padW - btnSz) * 0.5f);
        ImGui::Button(" ^ ", {btnSz, btnSz});
        if (ImGui::IsItemActive()) m_editorCam->SetMoveForward(true);

        // Row 2: Left / Back / Right
        float row2Y = ImGui::GetCursorPosY();
        ImGui::SetCursorPos({(padW - btnSz*3 - spacing*2)*0.5f, row2Y});

        ImGui::Button(" < ", {btnSz,btnSz});
        if (ImGui::IsItemActive()) m_editorCam->SetMoveLeft(true);
        ImGui::SameLine(0, spacing);
        ImGui::Button(" v ", {btnSz,btnSz});
        if (ImGui::IsItemActive()) m_editorCam->SetMoveBack(true);
        ImGui::SameLine(0, spacing);
        ImGui::Button(" > ", {btnSz,btnSz});
        if (ImGui::IsItemActive()) m_editorCam->SetMoveRight(true);

        ImGui::PopStyleColor(3);

        // Row 3: Up / Down (vertical)
        ImGui::PushStyleColor(ImGuiCol_Button,        {0.25f,0.25f,0.45f,0.95f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.35f,0.35f,0.60f,1.00f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {0.50f,0.50f,0.80f,1.00f});
        float row3Y = ImGui::GetCursorPosY();
        ImGui::SetCursorPos({(padW - btnSz*2 - spacing)*0.5f, row3Y});
        ImGui::Button("Up",  {btnSz,btnSz});
        if (ImGui::IsItemActive()) m_editorCam->SetMoveUp(true);
        ImGui::SameLine(0, spacing);
        ImGui::Button("Dn",  {btnSz,btnSz});
        if (ImGui::IsItemActive()) m_editorCam->SetMoveDown(true);
        ImGui::PopStyleColor(3);

        ImGui::End();
    }

    // ---- Look Pad (bottom right) ----
    void DrawLookPad(float sw, float sh) {
        if (!m_editorCam) return;

        float padW = 180.0f;
        float padH = 195.0f;
        float padX = sw - padW - 185.0f; // left of inspector
        float padY = sh - padH - 5.0f;

        ImGui::SetNextWindowPos({padX, padY}, ImGuiCond_Always);
        ImGui::SetNextWindowSize({padW, padH}, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.85f);
        ImGui::Begin("LOOK", nullptr,
            ImGuiWindowFlags_NoResize   |
            ImGuiWindowFlags_NoMove     |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar);

        float btnSz  = 58.0f;
        float spacing = 4.0f;

        ImGui::PushStyleColor(ImGuiCol_Button,        {0.35f,0.15f,0.55f,0.95f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.45f,0.25f,0.70f,1.00f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {0.60f,0.40f,0.90f,1.00f});

        // Row 1: Up button centred
        ImGui::SetCursorPosX((padW - btnSz) * 0.5f);
        ImGui::Button(" ^ ##lu", {btnSz, btnSz});
        if (ImGui::IsItemActive()) m_editorCam->SetLookUp(true);

        // Row 2: Left / Down / Right
        float row2Y = ImGui::GetCursorPosY();
        ImGui::SetCursorPos({(padW - btnSz*3 - spacing*2)*0.5f, row2Y});
        ImGui::Button(" < ##ll", {btnSz, btnSz});
        if (ImGui::IsItemActive()) m_editorCam->SetLookLeft(true);
        ImGui::SameLine(0, spacing);
        ImGui::Button(" v ##ld", {btnSz, btnSz});
        if (ImGui::IsItemActive()) m_editorCam->SetLookDown(true);
        ImGui::SameLine(0, spacing);
        ImGui::Button(" > ##lr", {btnSz, btnSz});
        if (ImGui::IsItemActive()) m_editorCam->SetLookRight(true);

        ImGui::PopStyleColor(3);

        ImGui::End();
    }

    // ---- Helpers ----
    void CreateObject(const std::string& name, const std::string& type) {
        if (!m_scene) return;
        GameObject* go = m_scene->CreateGameObject(name);
        if (type=="cube") {
            auto* mr=go->AddComponent<MeshRenderer>();
            mr->SetMeshData(PrimitiveMesh::CreateCube(1.0f));
            mr->SetMaterial(Material::Default());
        } else if (type=="sphere") {
            auto* mr=go->AddComponent<MeshRenderer>();
            mr->SetMeshData(PrimitiveMesh::CreateSphere(0.5f,20,20));
            mr->SetMaterial(Material::Default());
        } else if (type=="plane") {
            auto* mr=go->AddComponent<MeshRenderer>();
            mr->SetMeshData(PrimitiveMesh::CreatePlane(1,1,4,4));
            mr->SetMaterial(Material::Default());
        } else if (type=="cylinder") {
            auto* mr=go->AddComponent<MeshRenderer>();
            mr->SetMeshData(PrimitiveMesh::CreateCylinder(0.5f,1.0f,20));
            mr->SetMaterial(Material::Default());
        } else if (type=="pointlight") {
            auto* l=go->AddComponent<Light>();
            l->SetType(LightType::Point);
        }
        m_selected = go;
        GLRenderer::Get().SetSelectedObject(go);
    }

    Scene*        m_scene;
    GameObject*   m_selected;
    EditorCamera* m_editorCam;
    bool          m_playing;
    bool          m_showStats;
    bool          m_showGrid;
    bool          m_showMovepad;
    bool          m_showLookpad;
    int           m_transformMode; // 0=move 1=rotate 2=scale
    float         m_snapValue;
    bool          m_snapEnabled;
};

} // namespace Nova
