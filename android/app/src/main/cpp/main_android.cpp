#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include <android/native_window.h>
#include <EGL/eglplatform.h>
#include <cmath>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_android.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "NexusEngine", __VA_ARGS__)

struct Engine {
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    int width=0, height=0;
    bool running=false;
    float camX=0,camY=4,camZ=10;
    float yaw=-90,pitch=-20,moveSpd=5.0f;
    bool touching=false;
    float lastTX=0,lastTY=0;
};

static Engine g_engine;

void InitDisplay(Engine* e, ANativeWindow* w){
    const EGLint att[]={EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_BLUE_SIZE,8,EGL_GREEN_SIZE,8,EGL_RED_SIZE,8,EGL_DEPTH_SIZE,24,EGL_NONE};
    EGLint n; EGLConfig cfg;
    e->display=eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(e->display,nullptr,nullptr);
    eglChooseConfig(e->display,att,&cfg,1,&n);
    const EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    e->context=eglCreateContext(e->display,cfg,nullptr,ca);
    e->surface=eglCreateWindowSurface(e->display,cfg,(EGLNativeWindowType)w,nullptr);
    eglMakeCurrent(e->display,e->surface,e->surface,e->context);
    eglQuerySurface(e->display,e->surface,EGL_WIDTH,&e->width);
    eglQuerySurface(e->display,e->surface,EGL_HEIGHT,&e->height);
}

void GetFwd(float yaw,float pitch,float&fx,float&fy,float&fz){
    float yr=yaw*3.14159f/180,pr=pitch*3.14159f/180;
    fx=cosf(pr)*cosf(yr); fy=sinf(pr); fz=cosf(pr)*sinf(yr);
}

void RenderFrame(Engine* e){
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();
    float sw=e->width,sh=e->height;
    glViewport(0,0,e->width,e->height);
    glClearColor(0.08f,0.08f,0.12f,1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    // Title
    ImGui::SetNextWindowPos({sw*0.3f,2});
    ImGui::SetNextWindowSize({sw*0.4f,36});
    ImGui::SetNextWindowBgAlpha(0);
    ImGui::Begin("##t",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);
    ImGui::PushStyleColor(ImGuiCol_Text,{0.2f,0.6f,1.0f,1.0f});
    ImGui::Text("Nexus Engine 2.0");
    ImGui::PopStyleColor();
    ImGui::End();

    float padW=200,padH=210,btnSz=56,sp=4;
    float fx,fy,fz;
    GetFwd(e->yaw,e->pitch,fx,fy,fz);
    float spd=e->moveSpd*0.016f;

    // Move pad
    ImGui::SetNextWindowPos({5,sh-padH-5},ImGuiCond_Always);
    ImGui::SetNextWindowSize({padW,padH},ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.85f);
    ImGui::Begin("MOVE",nullptr,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoScrollbar);
    ImGui::SetCursorPosX((padW-btnSz)*0.5f);
    ImGui::Button(" ^ ##mf",{btnSz,btnSz});
    if(ImGui::IsItemActive()){e->camX+=fx*spd;e->camY+=fy*spd;e->camZ+=fz*spd;}
    float ry=ImGui::GetCursorPosY();
    ImGui::SetCursorPos({(padW-btnSz*3-sp*2)*0.5f,ry});
    ImGui::Button(" < ##ml",{btnSz,btnSz});
    if(ImGui::IsItemActive()){e->camX+=fz*spd;e->camZ-=fx*spd;}
    ImGui::SameLine(0,sp);
    ImGui::Button(" v ##mb",{btnSz,btnSz});
    if(ImGui::IsItemActive()){e->camX-=fx*spd;e->camY-=fy*spd;e->camZ-=fz*spd;}
    ImGui::SameLine(0,sp);
    ImGui::Button(" > ##mr",{btnSz,btnSz});
    if(ImGui::IsItemActive()){e->camX-=fz*spd;e->camZ+=fx*spd;}
    float ry2=ImGui::GetCursorPosY();
    ImGui::SetCursorPos({(padW-btnSz*2-sp)*0.5f,ry2});
    ImGui::Button("Up##mu",{btnSz,btnSz});
    if(ImGui::IsItemActive())e->camY+=spd;
    ImGui::SameLine(0,sp);
    ImGui::Button("Dn##md",{btnSz,btnSz});
    if(ImGui::IsItemActive())e->camY-=spd;
    ImGui::End();

    // Look pad
    ImGui::SetNextWindowPos({sw-padW-5,sh-padH-5},ImGuiCond_Always);
    ImGui::SetNextWindowSize({padW,padH},ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.85f);
    ImGui::Begin("LOOK",nullptr,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoScrollbar);
    float lspd=1.5f;
    ImGui::SetCursorPosX((padW-btnSz)*0.5f);
    ImGui::Button(" ^ ##lu",{btnSz,btnSz});
    if(ImGui::IsItemActive()){e->pitch+=lspd;if(e->pitch>89)e->pitch=89;}
    float lry=ImGui::GetCursorPosY();
    ImGui::SetCursorPos({(padW-btnSz*3-sp*2)*0.5f,lry});
    ImGui::Button(" < ##ll",{btnSz,btnSz});
    if(ImGui::IsItemActive())e->yaw-=lspd;
    ImGui::SameLine(0,sp);
    ImGui::Button(" v ##ld",{btnSz,btnSz});
    if(ImGui::IsItemActive()){e->pitch-=lspd;if(e->pitch<-89)e->pitch=-89;}
    ImGui::SameLine(0,sp);
    ImGui::Button(" > ##lr",{btnSz,btnSz});
    if(ImGui::IsItemActive())e->yaw+=lspd;
    ImGui::Spacing();ImGui::Separator();
    ImGui::Text("Speed: %.1f",e->moveSpd);
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##spd",&e->moveSpd,0.5f,20.0f);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    eglSwapBuffers(e->display,e->surface);
}

static int32_t HandleInput(android_app* app,AInputEvent* event){
    Engine* e=(Engine*)app->userData;
    if(AInputEvent_getType(event)==AINPUT_EVENT_TYPE_MOTION){
        ImGui_ImplAndroid_HandleInputEvent(event);
        ImGuiIO& io=ImGui::GetIO();
        if(!io.WantCaptureMouse){
            int action=AMotionEvent_getAction(event)&AMOTION_EVENT_ACTION_MASK;
            float x=AMotionEvent_getX(event,0),y=AMotionEvent_getY(event,0);
            if(action==AMOTION_EVENT_ACTION_DOWN){e->touching=true;e->lastTX=x;e->lastTY=y;}
            else if(action==AMOTION_EVENT_ACTION_MOVE&&e->touching){
                e->yaw+=(x-e->lastTX)*0.3f;
                e->pitch-=(y-e->lastTY)*0.3f;
                if(e->pitch>89)e->pitch=89;
                if(e->pitch<-89)e->pitch=-89;
                e->lastTX=x;e->lastTY=y;
            }else if(action==AMOTION_EVENT_ACTION_UP)e->touching=false;
        }
        return 1;
    }
    return 0;
}

static void HandleCmd(android_app* app,int32_t cmd){
    Engine* e=(Engine*)app->userData;
    if(cmd==APP_CMD_INIT_WINDOW&&app->window){
        InitDisplay(e,app->window);
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO().FontGlobalScale=2.0f;
        ImGui::StyleColorsDark();
        ImGui_ImplAndroid_Init(app->window);
        ImGui_ImplOpenGL3_Init("#version 300 es");
        e->running=true;
    }else if(cmd==APP_CMD_TERM_WINDOW){
        e->running=false;
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplAndroid_Shutdown();
        ImGui::DestroyContext();
        eglMakeCurrent(e->display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
        eglDestroyContext(e->display,e->context);
        eglDestroySurface(e->display,e->surface);
        eglTerminate(e->display);
    }
}

void android_main(android_app* app){
    app->userData=&g_engine;
    app->onAppCmd=HandleCmd;
    app->onInputEvent=HandleInput;
    while(true){
        int events;
        android_poll_source* source;
        while(ALooper_pollOnce(g_engine.running?0:-1,nullptr,&events,(void**)&source)>=0){
            if(source)source->process(app,source);
            if(app->destroyRequested)return;
        }
        if(g_engine.running)RenderFrame(&g_engine);
    }
}
