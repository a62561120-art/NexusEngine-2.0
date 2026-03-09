#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include <string>

#define LOG_TAG "NexusEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_android.h"
#include "imgui/backends/imgui_impl_opengl3.h"

static EGLDisplay g_Display = EGL_NO_DISPLAY;
static EGLSurface g_Surface = EGL_NO_SURFACE;
static EGLContext g_Context = EGL_NO_CONTEXT;
static bool g_Initialized = false;
static int g_Width = 0, g_Height = 0;

static bool InitEGL(ANativeWindow* window) {
    g_Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(g_Display, 0, 0);

    const EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
        EGL_DEPTH_SIZE, 16, EGL_NONE
    };
    EGLConfig config;
    EGLint numConfigs;
    eglChooseConfig(g_Display, attribs, &config, 1, &numConfigs);

    const EGLint ctx_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    g_Context = eglCreateContext(g_Display, config, EGL_NO_CONTEXT, ctx_attribs);
    g_Surface = eglCreateWindowSurface(g_Display, config, window, NULL);
    eglMakeCurrent(g_Display, g_Surface, g_Surface, g_Context);

    eglQuerySurface(g_Display, g_Surface, EGL_WIDTH, &g_Width);
    eglQuerySurface(g_Display, g_Surface, EGL_HEIGHT, &g_Height);

    LOGI("EGL initialized: %dx%d", g_Width, g_Height);
    return true;
}

static void InitImGui(ANativeWindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)g_Width, (float)g_Height);

    ImGui::StyleColorsDark();
    ImGui_ImplAndroid_Init(window);
    ImGui_ImplOpenGL3_Init("#version 300 es");
    LOGI("ImGui initialized");
}

static void ShutdownEGL() {
    if (g_Display != EGL_NO_DISPLAY) {
        eglMakeCurrent(g_Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (g_Context != EGL_NO_CONTEXT) eglDestroyContext(g_Display, g_Context);
        if (g_Surface != EGL_NO_SURFACE) eglDestroySurface(g_Display, g_Surface);
        eglTerminate(g_Display);
    }
    g_Display = EGL_NO_DISPLAY;
    g_Surface = EGL_NO_SURFACE;
    g_Context = EGL_NO_CONTEXT;
}

static void RenderFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    // Main editor window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)g_Width, (float)g_Height));
    ImGui::Begin("Nexus Engine 2.0", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

    ImGui::TextColored(ImVec4(0.2f,0.6f,1.0f,1.0f), "NEXUS ENGINE 2.0");
    ImGui::Separator();
    ImGui::Text("Running on Android!");
    ImGui::Text("Resolution: %dx%d", g_Width, g_Height);
    ImGui::Separator();
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

    ImGui::End();
    ImGui::Render();

    glViewport(0, 0, g_Width, g_Height);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    eglSwapBuffers(g_Display, g_Surface);
}

static int32_t HandleInput(android_app* app, AInputEvent* event) {
    return ImGui_ImplAndroid_HandleInputEvent(event);
}

static void HandleCmd(android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (app->window) {
                InitEGL(app->window);
                InitImGui(app->window);
                g_Initialized = true;
            }
            break;
        case APP_CMD_TERM_WINDOW:
            if (g_Initialized) {
                ImGui_ImplOpenGL3_Shutdown();
                ImGui_ImplAndroid_Shutdown();
                ImGui::DestroyContext();
                ShutdownEGL();
                g_Initialized = false;
            }
            break;
        case APP_CMD_WINDOW_RESIZED:
            eglQuerySurface(g_Display, g_Surface, EGL_WIDTH, &g_Width);
            eglQuerySurface(g_Display, g_Surface, EGL_HEIGHT, &g_Height);
            break;
    }
}

void android_main(android_app* app) {
    app->onAppCmd = HandleCmd;
    app->onInputEvent = HandleInput;

    while (true) {
        int events;
        android_poll_source* source;
        int result = ALooper_pollOnce(g_Initialized ? 0 : -1, nullptr, &events, (void**)&source);
        if (result == ALOOPER_POLL_ERROR) break;
        if (source) source->process(app, source);
        if (app->destroyRequested) break;
        if (g_Initialized) RenderFrame();
    }
}
