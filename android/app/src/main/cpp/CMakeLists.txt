cmake_minimum_required(VERSION 3.22.1)
project(nexusengine)

add_library(nexusengine SHARED
    main_android.cpp
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/backends/imgui_impl_android.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(nexusengine PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/imgui
    ${CMAKE_SOURCE_DIR}/imgui/backends
    ${ANDROID_NDK}/sources/android/native_app_glue
)

add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

target_compile_definitions(nexusengine PRIVATE ANDROID IMGUI_IMPL_OPENGL_ES3)

target_link_libraries(nexusengine
    native_app_glue
    android log EGL GLESv3 m
)
