# ============================================================
#  NovaEngine GL Edition — Makefile
#  Usage:  make          (build)
#          make clean    (clean)
#          make run      (build + run)
# ============================================================

CXX      = clang++
CXXFLAGS = -std=c++17 -O2 -Wall -Wno-unused-function -DGLFW_INCLUDE_NONE -Wno-unused-private-field \
           -I. -I./imgui -I./imgui/backends

# Libraries
LIBS = -lGL -lGLEW -lglfw -ldl -lpthread -lm

# ImGui sources
IMGUI_SRC = \
    imgui/imgui.cpp \
    imgui/imgui_draw.cpp \
    imgui/imgui_widgets.cpp \
    imgui/imgui_tables.cpp \
    imgui/backends/imgui_impl_glfw.cpp \
    imgui/backends/imgui_impl_opengl3.cpp

# Engine sources
ENGINE_SRC = \
    Engine/Renderer/Camera.cpp \
    Engine/Renderer/Light.cpp

# All sources
SRCS = main.cpp $(ENGINE_SRC) $(IMGUI_SRC)

TARGET = novaengine

all: $(TARGET)

$(TARGET): $(SRCS)
	@echo "Building NovaEngine GL Edition..."
	$(CXX) $(CXXFLAGS) $(SRCS) $(LIBS) -o $(TARGET)
	@echo "Build complete! Run with: ./$(TARGET)"

run: $(TARGET)
	DISPLAY=:1 ./$(TARGET)

clean:
	rm -f $(TARGET)
	@echo "Cleaned."
