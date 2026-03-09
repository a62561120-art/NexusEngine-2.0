#pragma once
// ============================================================
//  EGLWindow.h
//  Creates an OpenGL ES 2.0 surface on Android using EGL.
//  This is the bridge between the engine and the screen.
// ============================================================

// EGL and GLES headers - available on all Android devices
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/native_window.h>

#include <string>
#include "../Core/Logger.h"

namespace Nova {

struct WindowConfig {
    int         width       = 800;
    int         height      = 600;
    std::string title       = "NovaEngine";
    bool        fullscreen  = true;   // recommended for Android
    int         depthBits   = 16;
    int         stencilBits = 0;
};

// -------------------------------------------------------
// EGLWindow – manages the EGL display, surface and context.
// On Android you don't get a "window" from the OS the same
// way you do on PC. Instead EGL creates a rendering surface
// directly attached to the display.
// -------------------------------------------------------
class EGLWindow {
public:
    EGLWindow()
        : m_display(EGL_NO_DISPLAY),
          m_surface(EGL_NO_SURFACE),
          m_context(EGL_NO_CONTEXT),
          m_width(0), m_height(0),
          m_initialised(false)
    {}

    ~EGLWindow() { Shutdown(); }

    // Create EGL display + surface + context
    // Pass nullptr for nativeWindow to use the default display
    bool Init(const WindowConfig& cfg, EGLNativeWindowType nativeWindow = nullptr) {
        m_config = cfg;

        // --- 1. Get the default display ---
        m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (m_display == EGL_NO_DISPLAY) {
            NOVA_LOG_ERROR("EGLWindow", "eglGetDisplay failed");
            return false;
        }

        // --- 2. Initialise EGL ---
        EGLint major, minor;
        if (!eglInitialize(m_display, &major, &minor)) {
            NOVA_LOG_ERROR("EGLWindow", "eglInitialize failed");
            return false;
        }
        NOVA_LOG_INFO("EGLWindow", "EGL " + std::to_string(major) +
                      "." + std::to_string(minor) + " initialised");

        // --- 3. Choose a config (pixel format) ---
        EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
            EGL_RED_SIZE,        8,
            EGL_GREEN_SIZE,      8,
            EGL_BLUE_SIZE,       8,
            EGL_ALPHA_SIZE,      8,
            EGL_DEPTH_SIZE,      cfg.depthBits,
            EGL_STENCIL_SIZE,    cfg.stencilBits,
            EGL_NONE
        };
        EGLConfig eglConfig;
        EGLint    numConfigs;
        if (!eglChooseConfig(m_display, attribs, &eglConfig, 1, &numConfigs)
            || numConfigs == 0)
        {
            NOVA_LOG_ERROR("EGLWindow", "eglChooseConfig failed");
            return false;
        }

        // --- 4. Create window surface ---
        m_surface = eglCreateWindowSurface(m_display, eglConfig,
                                           nativeWindow, nullptr);
        if (m_surface == EGL_NO_SURFACE) {
            NOVA_LOG_ERROR("EGLWindow", "eglCreateWindowSurface failed: " +
                           std::to_string(eglGetError()));
            return false;
        }

        // --- 5. Create OpenGL ES 2.0 context ---
        EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
        m_context = eglCreateContext(m_display, eglConfig,
                                     EGL_NO_CONTEXT, ctxAttribs);
        if (m_context == EGL_NO_CONTEXT) {
            NOVA_LOG_ERROR("EGLWindow", "eglCreateContext failed");
            return false;
        }

        // --- 6. Make context current ---
        if (!eglMakeCurrent(m_display, m_surface, m_surface, m_context)) {
            NOVA_LOG_ERROR("EGLWindow", "eglMakeCurrent failed");
            return false;
        }

        // --- 7. Query actual surface size ---
        eglQuerySurface(m_display, m_surface, EGL_WIDTH,  &m_width);
        eglQuerySurface(m_display, m_surface, EGL_HEIGHT, &m_height);

        NOVA_LOG_INFO("EGLWindow", "Surface size: " +
                      std::to_string(m_width) + "x" + std::to_string(m_height));

        // Optional: disable vsync for max FPS (comment out to enable vsync)
        // eglSwapInterval(m_display, 0);

        m_initialised = true;
        return true;
    }

    // Call at the end of every frame to show the rendered image
    void SwapBuffers() {
        if (m_initialised)
            eglSwapBuffers(m_display, m_surface);
    }

    // Destroy EGL resources
    void Shutdown() {
        if (m_display != EGL_NO_DISPLAY) {
            eglMakeCurrent(m_display, EGL_NO_SURFACE,
                           EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (m_context != EGL_NO_CONTEXT) eglDestroyContext(m_display, m_context);
            if (m_surface != EGL_NO_SURFACE) eglDestroySurface(m_display, m_surface);
            eglTerminate(m_display);
        }
        m_display     = EGL_NO_DISPLAY;
        m_surface     = EGL_NO_SURFACE;
        m_context     = EGL_NO_CONTEXT;
        m_initialised = false;
    }

    bool  IsInitialised() const { return m_initialised; }
    int   GetWidth()      const { return m_width; }
    int   GetHeight()     const { return m_height; }
    float GetAspect()     const {
        return m_height > 0 ? (float)m_width / m_height : 1.0f;
    }

    EGLDisplay GetDisplay() const { return m_display; }
    EGLSurface GetSurface() const { return m_surface; }
    EGLContext GetContext() const { return m_context; }

private:
    EGLDisplay   m_display;
    EGLSurface   m_surface;
    EGLContext   m_context;
    EGLint       m_width, m_height;
    bool         m_initialised;
    WindowConfig m_config;
};

} // namespace Nova
