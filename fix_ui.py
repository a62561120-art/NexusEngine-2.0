with open('android/app/src/main/cpp/main_android.cpp', 'r') as f:
    s = f.read()

# Fix 1: ImGui ID conflict in hierarchy - use PushID/PopID
old_hierarchy = '''    scene->ForEach([&](GameObject* go){
        if (go->GetName()=="EditorCamera") return;              bool sel=(go==selected);
        if (sel) ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(1.f,0.8f,0.2f,1.f));
        if (ImGui::Button(go->GetName().c_str(),ImVec2(-1,40))) selected=go;                                            if (sel) ImGui::PopStyleColor();
    });'''

new_hierarchy = '''    int goIdx=0;
    scene->ForEach([&](GameObject* go){
        if (go->GetName()=="EditorCamera") return;
        bool sel=(go==selected);
        ImGui::PushID(goIdx++);
        if (sel) ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(1.f,0.8f,0.2f,1.f));
        if (ImGui::Button(go->GetName().c_str(),ImVec2(-1,40))) selected=go;
        if (sel) ImGui::PopStyleColor();
        ImGui::PopID();
    });'''

if old_hierarchy in s:
    s = s.replace(old_hierarchy, new_hierarchy)
    print("Fixed hierarchy IDs")
else:
    # Try simpler replacement
    s = s.replace(
        'scene->ForEach([&](GameObject* go){\n        if (go->GetName()=="EditorCamera") return;',
        'int goIdx=0;\n    scene->ForEach([&](GameObject* go){\n        if (go->GetName()=="EditorCamera") return;\n        ImGui::PushID(goIdx++);'
    )
    s = s.replace(
        'if (ImGui::Button(go->GetName().c_str(),ImVec2(-1,40))) selected=go;                                            if (sel) ImGui::PopStyleColor();\n    });',
        'if (ImGui::Button(go->GetName().c_str(),ImVec2(-1,40))) selected=go;\n        if (sel) ImGui::PopStyleColor();\n        ImGui::PopID();\n    });'
    )
    print("Fixed hierarchy IDs (alt)")

# Fix 2: Move pad - IsItemActive doesn't work well on Android, use IsItemHeld instead
s = s.replace('ImGui::IsItemActive()) edCam.moveF=true;', 'ImGui::IsItemActive()) edCam.moveF=true; else edCam.moveF=false;')

# Better fix: replace the whole move pad with proper hold detection
old_move = '''    ImGui::Begin("MOVE",nullptr,
        ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|
        ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar);
    ImGui::Text("MOVE");                                    float bsz=50.f;
    ImGui::SetCursorPos(ImVec2(bsz,5));                     if(ImGui::Button("^##mf",ImVec2(bsz,bsz))&&ImGui::IsItemActive()) edCam.moveF=true;                             ImGui::SetCursorPos(ImVec2(0,bsz));
    if(ImGui::Button("<##ml",ImVec2(bsz,bsz))&&ImGui::IsItemActive()) edCam.moveL=true;
    ImGui::SameLine();
    if(ImGui::Button("v##md",ImVec2(bsz,bsz))&&ImGui::IsItemActive()) edCam.moveD=true;                             ImGui::SameLine();
    if(ImGui::Button(">##mr",ImVec2(bsz,bsz))&&ImGui::IsItemActive()) edCam.moveR=true;
    ImGui::SetCursorPos(ImVec2(0,bsz*2));
    if(ImGui::Button("Up##mu",ImVec2(bsz,bsz))&&ImGui::IsItemActive()) edCam.moveU=true;
    ImGui::SameLine();                                      if(ImGui::Button("Dn##mb",ImVec2(bsz,bsz))&&ImGui::IsItemActive()) edCam.moveB=true;
    ImGui::End();'''

new_move = '''    ImGui::Begin("MOVE",nullptr,
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

if old_move in s:
    s = s.replace(old_move, new_move)
    print("Fixed move pad")
else:
    print("WARNING: move pad not found for replacement - doing line by line")
    s = s.replace('if(ImGui::Button("^##mf",ImVec2(bsz,bsz))&&ImGui::IsItemActive()) edCam.moveF=true;', 'ImGui::Button("^##mf",ImVec2(bsz,bsz)); if(ImGui::IsItemActive()) edCam.moveF=true;')
    s = s.replace('if(ImGui::Button("<##ml",ImVec2(bsz,bsz))&&ImGui::IsItemActive()) edCam.moveL=true;', 'ImGui::Button("<##ml",ImVec2(bsz,bsz)); if(ImGui::IsItemActive()) edCam.moveL=true;')
    s = s.replace('if(ImGui::Button("v##md",ImVec2(bsz,bsz))&&ImGui::IsItemActive()) edCam.moveD=true;', 'ImGui::Button("v##md",ImVec2(bsz,bsz)); if(ImGui::IsItemActive()) edCam.moveD=true;')
    s = s.replace('if(ImGui::Button(">##mr",ImVec2(bsz,bsz))&&ImGui::IsItemActive()) edCam.moveR=true;', 'ImGui::Button(">##mr",ImVec2(bsz,bsz)); if(ImGui::IsItemActive()) edCam.moveR=true;')
    s = s.replace('if(ImGui::Button("Up##mu",ImVec2(bsz,bsz))&&ImGui::IsItemActive()) edCam.moveU=true;', 'ImGui::Button("Up##mu",ImVec2(bsz,bsz)); if(ImGui::IsItemActive()) edCam.moveU=true;')
    s = s.replace('if(ImGui::Button("Dn##mb",ImVec2(bsz,bsz))&&ImGui::IsItemActive()) edCam.moveB=true;', 'ImGui::Button("Dn##mb",ImVec2(bsz,bsz)); if(ImGui::IsItemActive()) edCam.moveB=true;')
    print("Fixed move pad (line by line)")

# Fix 3: Remove LOOK pad entirely
import re
look_pattern = r'    ImGui::SetNextWindowPos\(ImVec2\(\(float\)w-padSz-20.*?ImGui::End\(\);\n\}'
s2 = re.sub(look_pattern, '}', s, flags=re.DOTALL)
if s2 != s:
    s = s2
    print("Removed look pad")
else:
    print("WARNING: look pad not found by regex")

# Fix 4: Add File and View menu to title bar
old_title = '''    ImGui::TextColored(ImVec4(0.2f,0.6f,1.f,1.f),"NEXUS ENGINE 2.0");
    ImGui::SameLine(0,30);
    ImGui::Text("FPS: %.0f", fps);
    ImGui::End();'''

new_title = '''    ImGui::TextColored(ImVec4(0.2f,0.6f,1.f,1.f),"NEXUS ENGINE 2.0");
    ImGui::SameLine(0,20);
    if(ImGui::BeginMenu("File")) {
        if(ImGui::MenuItem("New Scene")) { scene->Clear(); }
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
    ImGui::Text("FPS: %.0f", fps);
    ImGui::End();'''

if old_title in s:
    s = s.replace(old_title, new_title)
    print("Added File/View menus")
else:
    print("WARNING: title bar not found")

with open('android/app/src/main/cpp/main_android.cpp', 'w') as f:
    f.write(s)
print("All fixes applied!")
