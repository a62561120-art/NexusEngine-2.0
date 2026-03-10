with open('android/app/src/main/cpp/main_android.cpp', 'r') as f:
    s = f.read()

# Add joystick state variables after g_viewportDrag
s = s.replace(
    'static float g_lastTX=0,g_lastTY=0;\nstatic bool  g_viewportDrag=false;',
    '''static float g_lastTX=0,g_lastTY=0;
static bool  g_viewportDrag=false;
// Joystick state
static float g_joyBaseX=0,g_joyBaseY=0;   // where finger touched down
static float g_joyDX=0,g_joyDY=0;         // current delta
static bool  g_joyActive=false;
static int   g_joyPointer=-1;              // which pointer is joystick'''
)

# Replace the entire MOVE pad with joystick in DrawEditorUI
old_move = '''    ImGui::Begin("MOVE",nullptr,
        ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|
        ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar);
    ImGui::Text("MOVE");
    float bsz=50.f;
    ImGui::SetCursorPos(ImVec2(bsz,5));
    ImGui::Button("^##mf",ImVec2(bsz,bsz)); if(ImGui::IsItemActive()) edCam.moveF=true;
    ImGui::SetCursorPos(ImVec2(0,bsz));
    ImGui::Button("<##ml",ImVec2(bsz,bsz)); if(ImGui::IsItemActive()) edCam.moveL=true;
    ImGui::SameLine();
    ImGui::Button("v##md",ImVec2(bsz,bsz)); if(ImGui::IsItemActive()) edCam.moveD=true;
    ImGui::SameLine();
    ImGui::Button(">##mr",ImVec2(bsz,bsz)); if(ImGui::IsItemActive()) edCam.moveR=true;
    ImGui::SetCursorPos(ImVec2(0,bsz*2));
    ImGui::Button("Up##mu",ImVec2(bsz,bsz)); if(ImGui::IsItemActive()) edCam.moveU=true;
    ImGui::SameLine();
    ImGui::Button("Dn##mb",ImVec2(bsz,bsz)); if(ImGui::IsItemActive()) edCam.moveB=true;
    ImGui::End();'''

new_move = '''    // ---- Joystick ----
    float joySize=130.f;
    float joyX=20.f, joyY=(float)h-joySize-20.f;
    ImGui::SetNextWindowPos(ImVec2(joyX,joyY));
    ImGui::SetNextWindowSize(ImVec2(joySize,joySize));
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGui::Begin("##joy",nullptr,
        ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|
        ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar|
        ImGuiWindowFlags_NoInputs);
    ImDrawList* dl=ImGui::GetWindowDrawList();
    ImVec2 wp=ImGui::GetWindowPos();
    float cx=wp.x+joySize*0.5f, cy=wp.y+joySize*0.5f;
    float outerR=joySize*0.45f, innerR=joySize*0.18f;
    // outer ring
    dl->AddCircle(ImVec2(cx,cy),outerR,IM_COL32(100,150,255,120),32,2.f);
    // inner knob - offset by joystick delta clamped to outerR
    float kx=cx, ky=cy;
    if(g_joyActive){
        float dx=g_joyDX, dy=g_joyDY;
        float len=sqrtf(dx*dx+dy*dy);
        if(len>outerR){dx=dx/len*outerR;dy=dy/len*outerR;}
        kx+=dx; ky+=dy;
    }
    dl->AddCircleFilled(ImVec2(kx,ky),innerR,IM_COL32(100,180,255,200));
    ImGui::End();
    // Apply joystick movement - moves in camera-facing direction
    if(g_joyActive){
        float dx=g_joyDX, dy=g_joyDY;
        float len=sqrtf(dx*dx+dy*dy);
        if(len>8.f){ // deadzone
            float nx=dx/len, ny=dy/len; // normalized -1..1
            float strength=std::min(len/60.f,1.f);
            // nx = strafe left/right, ny = forward/back (screen Y is inverted)
            if(ny < -0.3f) edCam.moveF=true;  // up = forward
            if(ny >  0.3f) edCam.moveB=true;  // down = backward
            if(nx < -0.3f) edCam.moveL=true;  // left = strafe left
            if(nx >  0.3f) edCam.moveR=true;  // right = strafe right
        }
    }'''

if old_move in s:
    s = s.replace(old_move, new_move)
    print("Replaced move pad with joystick")
else:
    print("ERROR: move pad not found!")

# Now handle joystick touch input in HandleInput
# We need to detect touches in the joystick area and track them separately
old_input_motion = '''    if (AInputEvent_getType(event)==AINPUT_EVENT_TYPE_MOTION) {
        int action=AMotionEvent_getAction(event)&AMOTION_EVENT_ACTION_MASK;
        float tx=AMotionEvent_getX(event,0);
        float ty=AMotionEvent_getY(event,0);

        // Only drag if not in UI panels
        bool inHierarchy = tx<200 && ty>50 && ty<g_vpH*0.5f+50;
        bool inInspector = tx>g_vpW-210 && ty>50 && ty<g_vpH*0.5f+50;
        bool inMovePad   = tx<200 && ty>g_vpH-180;
        bool inLookPad   = tx>g_vpW-210 && ty>g_vpH-180;
        bool inUI = inHierarchy||inInspector||inMovePad||inLookPad;

        if (action==AMOTION_EVENT_ACTION_DOWN && !inUI) {
            g_viewportDrag=true; g_lastTX=tx; g_lastTY=ty;
        } else if (action==AMOTION_EVENT_ACTION_MOVE && g_viewportDrag) {
            float dx=tx-g_lastTX, dy=ty-g_lastTY;
            g_edCam.OnTouchDrag(dx,dy);
            g_lastTX=tx; g_lastTY=ty;
        } else if (action==AMOTION_EVENT_ACTION_UP) {
            g_viewportDrag=false;
        }
    }'''

new_input_motion = '''    if (AInputEvent_getType(event)==AINPUT_EVENT_TYPE_MOTION) {
        int action=AMotionEvent_getAction(event)&AMOTION_EVENT_ACTION_MASK;
        int ptrIdx=(AMotionEvent_getAction(event)&AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)>>AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        int ptrCount=(int)AMotionEvent_getPointerCount(event);

        // Joystick zone: bottom-left 160x160
        auto inJoyZone=[&](float x,float y)->bool{
            return x<160.f && y>(float)g_vpH-160.f;
        };

        // Process all pointers for joystick and viewport
        for(int i=0;i<ptrCount;i++){
            float tx=AMotionEvent_getX(event,i);
            float ty=AMotionEvent_getY(event,i);
            int pid=AMotionEvent_getPointerId(event,i);

            bool isDown=(action==AMOTION_EVENT_ACTION_DOWN)||(action==AMOTION_EVENT_ACTION_POINTER_DOWN&&i==ptrIdx);
            bool isUp=(action==AMOTION_EVENT_ACTION_UP)||(action==AMOTION_EVENT_ACTION_POINTER_UP&&i==ptrIdx);
            bool isMove=(action==AMOTION_EVENT_ACTION_MOVE);

            if(isDown){
                if(inJoyZone(tx,ty)){
                    g_joyActive=true;
                    g_joyPointer=pid;
                    g_joyBaseX=tx; g_joyBaseY=ty;
                    g_joyDX=0; g_joyDY=0;
                } else {
                    bool inHierarchy=tx<200&&ty>50&&ty<(float)g_vpH*0.5f+50;
                    bool inInspector=tx>(float)g_vpW-210&&ty>50&&ty<(float)g_vpH*0.5f+50;
                    if(!inHierarchy&&!inInspector){
                        g_viewportDrag=true;
                        g_lastTX=tx; g_lastTY=ty;
                    }
                }
            } else if(isMove){
                if(g_joyActive&&pid==g_joyPointer){
                    g_joyDX=tx-g_joyBaseX;
                    g_joyDY=ty-g_joyBaseY;
                } else if(g_viewportDrag&&!g_joyActive){
                    float dx=tx-g_lastTX, dy=ty-g_lastTY;
                    g_edCam.OnTouchDrag(dx,dy);
                    g_lastTX=tx; g_lastTY=ty;
                }
            } else if(isUp){
                if(g_joyActive&&pid==g_joyPointer){
                    g_joyActive=false;
                    g_joyPointer=-1;
                    g_joyDX=0; g_joyDY=0;
                } else {
                    g_viewportDrag=false;
                }
            }
        }
    }'''

if old_input_motion in s:
    s = s.replace(old_input_motion, new_input_motion)
    print("Replaced input handler with multi-touch joystick")
else:
    print("ERROR: input handler not found!")

with open('android/app/src/main/cpp/main_android.cpp', 'w') as f:
    f.write(s)
print("Done!")
