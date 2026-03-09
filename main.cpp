// ============================================================
//  main.cpp  —  Nexus Engine 2.0
//  Full graphical game engine editor running on Termux + VNC
//
//  Controls:
//   Right-click + drag  = look around
//   WASD               = fly camera
//   Q/E                = move down/up
//   Scroll wheel       = zoom
//   Middle mouse drag  = pan
//   F                  = focus on selected object
//   Delete             = delete selected object
//   Shift + WASD       = fast fly
//
//  Build:
//   cd ~/NovaEngine
//   make
// ============================================================

// Suppress GLEW/GLFW include order warnings
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// ImGui
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

// Engine
#include "Nova.h"
#include "Engine/Renderer/GLRenderer.h"
#include "Engine/Editor/EditorUI.h"
#include "Engine/Editor/EditorCamera.h"

#include <iostream>
#include <string>

using namespace Nova;

// ---- Globals ----
static EditorCamera  g_editorCam;
static EditorUI      g_editorUI;
static GLFWwindow*   g_window = nullptr;

// ---- GLFW Callbacks ----
void OnFramebufferResize(GLFWwindow*, int w, int h) {
    GLRenderer::Get().SetViewport(w, h);
    g_editorCam.SetAspect((float)w / std::max(h, 1));
}

void OnMouseButton(GLFWwindow*, int button, int action, int) {
    ImGuiIO& io = ImGui::GetIO();
    g_editorCam.OnMouseButton(button, action, io.WantCaptureMouse);
}

void OnMouseMove(GLFWwindow*, double x, double y) {
    g_editorCam.OnMouseMove(x, y);
}

void OnScroll(GLFWwindow*, double, double dy) {
    ImGuiIO& io = ImGui::GetIO();
    g_editorCam.OnScroll(dy, io.WantCaptureMouse);
}

void OnKey(GLFWwindow* window, int key, int, int action, int) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        // Focus on selected object
        GameObject* sel = g_editorUI.GetSelected();
        if (sel) g_editorCam.FocusOn(sel);
    }

    if (key == GLFW_KEY_DELETE && action == GLFW_PRESS) {
        // Delete selected object
        GameObject* sel = g_editorUI.GetSelected();
        Scene* scene = Engine::Get().GetScene();
        if (sel && scene) {
            scene->DestroyGameObject(sel);
            GLRenderer::Get().SetSelectedObject(nullptr);
        }
    }
}

// ---- Build demo scene ----
void BuildDemoScene(Scene* scene) {
    // Directional light (sun)
    GameObject* sun = scene->CreateGameObject("Sun");
    sun->GetTransform()->SetEulerAngles({50, -40, 0});
    Light* sunLight = sun->AddComponent<Light>();
    sunLight->SetType(LightType::Directional);
    sunLight->SetColor({1.0f, 0.92f, 0.80f});
    sunLight->SetIntensity(1.1f);

    // Fill light
    GameObject* fill = scene->CreateGameObject("FillLight");
    fill->GetTransform()->SetEulerAngles({20, 130, 0});
    Light* fillLight = fill->AddComponent<Light>();
    fillLight->SetType(LightType::Directional);
    fillLight->SetColor({0.4f, 0.5f, 0.8f});
    fillLight->SetIntensity(0.4f);

    // Ground plane
    GameObject* ground = scene->CreateGameObject("Ground");
    ground->GetTransform()->SetScale({12, 1, 12});
    MeshRenderer* gMR = ground->AddComponent<MeshRenderer>();
    gMR->SetMeshData(PrimitiveMesh::CreatePlane(1,1,8,8));
    Material gMat = Material::Colored({0.20f, 0.38f, 0.20f});
    gMat.name = "Ground";
    gMR->SetMaterial(gMat);

    // Red cube
    GameObject* cube = scene->CreateGameObject("Cube");
    cube->GetTransform()->SetPosition({0, 0.5f, 0});
    MeshRenderer* cMR = cube->AddComponent<MeshRenderer>();
    cMR->SetMeshData(PrimitiveMesh::CreateCube(1.0f));
    Material cMat = Material::Colored({0.85f, 0.22f, 0.18f});
    cMat.specular  = {0.8f, 0.8f, 0.8f};
    cMat.shininess = 64.0f;
    cMat.name = "RedMaterial";
    cMR->SetMaterial(cMat);

    // Blue sphere
    GameObject* sphere = scene->CreateGameObject("Sphere");
    sphere->GetTransform()->SetPosition({-2.5f, 0.6f, 0});
    MeshRenderer* sMR = sphere->AddComponent<MeshRenderer>();
    sMR->SetMeshData(PrimitiveMesh::CreateSphere(0.6f, 24, 24));
    Material sMat = Material::Colored({0.18f, 0.48f, 1.0f});
    sMat.shininess = 80.0f;
    sMat.name = "BlueMaterial";
    sMR->SetMaterial(sMat);

    // Yellow cylinder
    GameObject* cyl = scene->CreateGameObject("Cylinder");
    cyl->GetTransform()->SetPosition({2.5f, 1.0f, 0});
    MeshRenderer* yMR = cyl->AddComponent<MeshRenderer>();
    yMR->SetMeshData(PrimitiveMesh::CreateCylinder(0.5f, 2.0f, 24));
    Material yMat = Material::Colored({0.95f, 0.78f, 0.1f});
    yMat.shininess = 32.0f;
    yMat.name = "YellowMaterial";
    yMR->SetMaterial(yMat);

    // Green sphere
    GameObject* gs = scene->CreateGameObject("GreenSphere");
    gs->GetTransform()->SetPosition({0, 0.7f, -2.5f});
    MeshRenderer* gsMR = gs->AddComponent<MeshRenderer>();
    gsMR->SetMeshData(PrimitiveMesh::CreateSphere(0.7f, 20, 20));
    Material gsMat = Material::Colored({0.18f, 0.82f, 0.28f});
    gsMat.shininess = 48.0f;
    gsMat.name = "GreenMaterial";
    gsMR->SetMaterial(gsMat);

    // White sphere on top of cube (child)
    GameObject* top = scene->CreateGameObject("TopSphere");
    top->GetTransform()->SetPosition({0, 2.0f, 0});
    top->GetTransform()->SetScale({0.4f, 0.4f, 0.4f});
    MeshRenderer* tMR = top->AddComponent<MeshRenderer>();
    tMR->SetMeshData(PrimitiveMesh::CreateSphere(0.5f, 16, 16));
    Material tMat = Material::Colored({0.95f, 0.95f, 0.95f});
    tMat.shininess = 128.0f;
    tMat.specular  = {1.0f, 1.0f, 1.0f};
    tMat.name = "WhiteMaterial";
    tMR->SetMaterial(tMat);
}

// ---- Main ----
int main() {
    std::cout << "============================================\n";
    std::cout << "   Nexus Engine 2.0\n";
    std::cout << "   Graphical Game Engine Editor\n";
    std::cout << "============================================\n\n";

    // Init GLFW
    if (!glfwInit()) {
        std::cerr << "GLFW init failed\n";
        return 1;
    }

    // OpenGL 3.3 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA

    // Create window
    const int WIN_W = 960, WIN_H = 600;
    g_window = glfwCreateWindow(WIN_W, WIN_H,
        "Nexus Engine 2.0", nullptr, nullptr);
    if (!g_window) {
        // Fallback: try OpenGL 2.1 for older Mesa
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);
        g_window = glfwCreateWindow(WIN_W, WIN_H,
            "Nexus Engine 2.0", nullptr, nullptr);
        if (!g_window) {
            std::cerr << "Failed to create window\n";
            glfwTerminate();
            return 1;
        }
        std::cout << "Using OpenGL 2.1 fallback\n";
    }

    glfwMakeContextCurrent(g_window);
    glfwSwapInterval(1); // VSync

    // Register callbacks
    glfwSetFramebufferSizeCallback(g_window, OnFramebufferResize);
    glfwSetMouseButtonCallback(g_window, OnMouseButton);
    glfwSetCursorPosCallback(g_window, OnMouseMove);
    glfwSetScrollCallback(g_window, OnScroll);
    glfwSetKeyCallback(g_window, OnKey);

    // Init engine
    EngineConfig cfg;
    cfg.appName  = "Nexus Engine 2.0";
    cfg.logLevel = LogLevel::Info;
    Engine::Get().Init(cfg);

    // Init renderer
    if (!GLRenderer::Get().Init(WIN_W, WIN_H)) {
        std::cerr << "GLRenderer init failed\n";
        return 1;
    }
    GLRenderer::Get().SetAmbient(0.08f, 0.08f, 0.13f);

    // Build scene
    Scene* scene = Engine::Get().GetScene();
    BuildDemoScene(scene);

    // Editor camera
    float aspect = (float)WIN_W / WIN_H;
    g_editorCam.Init(scene, aspect);
    GLRenderer::Get().SetActiveCamera(g_editorCam.GetCamera());

    // Editor UI
    g_editorUI.Init(g_window, scene, &g_editorCam);
    g_editorUI.SetScene(scene);

    g_editorUI.OnPlay = [&]() {
        NOVA_LOG_INFO("Editor","Play mode started");
        Engine::Get().StartSystems();
    };
    g_editorUI.OnStop = [&]() {
        NOVA_LOG_INFO("Editor","Play mode stopped");
    };

    Engine::Get().StartSystems();

    std::cout << "Engine started! Check AVNC for the window.\n";
    std::cout << "Right-click + drag to look, WASD to fly.\n";

    // ---- Main loop ----
    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(g_window)) {
        glfwPollEvents();

        // Delta time
        double now = glfwGetTime();
        float dt   = (float)(now - lastTime);
        lastTime   = now;
        if (dt > 0.1f) dt = 0.1f; // clamp spike
        Time::Get().Tick();

        // Camera
        ImGuiIO& io = ImGui::GetIO();
        g_editorCam.Update(g_window, dt, io.WantCaptureMouse || io.WantCaptureKeyboard);

        // Flush destroyed objects
        // flush handled by engine

        // Begin ImGui frame
        g_editorUI.BeginFrame();

        // Render 3D scene
        auto cc = g_editorCam.GetCamera()->GetClearColor();
        GLRenderer::Get().BeginFrame(cc.r, cc.g, cc.b);
        GLRenderer::Get().RenderScene(scene);

        // Draw editor UI on top
        int fbW, fbH;
        glfwGetFramebufferSize(g_window, &fbW, &fbH);
        g_editorUI.Render();

        // End ImGui frame
        g_editorUI.EndFrame();

        glfwSwapBuffers(g_window);
    }

    // Cleanup
    g_editorUI.Shutdown();
    Engine::Get().Shutdown();
    glfwDestroyWindow(g_window);
    glfwTerminate();

    std::cout << "Engine shut down cleanly.\n";
    return 0;
}
