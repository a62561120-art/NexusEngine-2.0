#pragma once
#include "../Scene/Scene.h"
#include "../Core/GameObject.h"
#include "../Core/Transform.h"
#include <iostream>
#include <string>
#include <vector>
#include <functional>

namespace Nova {

// -------------------------------------------------------
// EditorHierarchy – prints/manages the scene hierarchy
// to stdout for a console "editor" experience.
// -------------------------------------------------------
class EditorHierarchy {
public:
    EditorHierarchy(Scene* scene) : m_scene(scene), m_selected(nullptr) {}

    void SetScene(Scene* scene) { m_scene = scene; m_selected = nullptr; }
    Scene* GetScene() const { return m_scene; }

    GameObject* GetSelected() const { return m_selected; }
    void SelectByName(const std::string& name) {
        if (!m_scene) return;
        m_selected = m_scene->Find(name);
        if (!m_selected) std::cout << "[Hierarchy] Object not found: " << name << "\n";
    }
    void SelectByID(EntityID id) {
        if (!m_scene) return;
        m_selected = m_scene->FindByID(id);
    }
    void Deselect() { m_selected = nullptr; }

    // Print the full hierarchy tree
    void PrintHierarchy() const {
        if (!m_scene) { std::cout << "(No scene)\n"; return; }
        std::cout << "\n=== Hierarchy: " << m_scene->GetName() << " ===\n";
        auto roots = m_scene->GetRootObjects();
        for (auto* go : roots)
            PrintObject(go, 0);
        std::cout << "=================================\n\n";
    }

    // Create an object in the current scene
    GameObject* CreateEmpty(const std::string& name = "GameObject") {
        if (!m_scene) return nullptr;
        return m_scene->CreateGameObject(name);
    }

    GameObject* CreateChild(const std::string& name = "Child") {
        if (!m_scene || !m_selected) {
            std::cout << "[Hierarchy] No object selected to parent under.\n";
            return nullptr;
        }
        return m_scene->CreateChildObject(m_selected, name);
    }

    void DeleteSelected() {
        if (!m_selected || !m_scene) return;
        std::cout << "[Hierarchy] Deleting: " << m_selected->GetName() << "\n";
        m_scene->DestroyGameObject(m_selected);
        m_selected = nullptr;
    }

private:
    void PrintObject(GameObject* go, int depth) const {
        std::string indent(depth * 2, ' ');
        std::string marker = (go == m_selected) ? ">> " : "   ";
        std::cout << marker << indent
                  << "[" << go->GetID() << "] "
                  << go->GetName()
                  << (go->IsActive() ? "" : " (INACTIVE)")
                  << "\n";
        for (auto* child : go->GetTransform()->GetChildren())
            if (child->GetOwner())
                PrintObject(child->GetOwner(), depth + 1);
    }

    Scene*      m_scene;
    GameObject* m_selected;
};

// -------------------------------------------------------
// EditorInspector – displays component data for the
// selected object.
// -------------------------------------------------------
class EditorInspector {
public:
    EditorInspector() {}

    void Inspect(const GameObject* go) const {
        if (!go) { std::cout << "(Nothing selected)\n"; return; }

        std::cout << "\n=== Inspector: " << go->GetName() << " ===\n";
        std::cout << "  ID:     " << go->GetID()     << "\n";
        std::cout << "  Tag:    " << go->GetTag()     << "\n";
        std::cout << "  Layer:  " << go->GetLayer()   << "\n";
        std::cout << "  Active: " << (go->IsActive() ? "Yes" : "No") << "\n";

        // Transform
        const Transform* t = go->GetTransform();
        const Vector3& pos = t->GetPosition();
        Vector3 euler      = t->GetEulerAngles();
        const Vector3& scl = t->GetScale();

        std::cout << "\n  [Transform]\n";
        std::cout << "    Position: " << pos.ToString()   << "\n";
        std::cout << "    Rotation: " << euler.ToString() << " deg\n";
        std::cout << "    Scale:    " << scl.ToString()   << "\n";
        if (t->GetParent())
            std::cout << "    Parent:   "
                      << (t->GetParent()->GetOwner()
                            ? t->GetParent()->GetOwner()->GetName()
                            : "?") << "\n";
        if (t->HasChildren())
            std::cout << "    Children: " << t->ChildCount() << "\n";

        // Other components
        int compCount = go->GetComponentCount();
        for (int i = 0; i < compCount; ++i) {
            Component* c = go->GetComponentByIndex(i);
            if (!c) continue;
            std::cout << "\n  [" << c->GetTypeName() << "]\n";
            std::cout << "    Enabled: " << (c->IsEnabled() ? "Yes" : "No") << "\n";
            PrintComponentDetails(c);
        }
        std::cout << "==================================\n\n";
    }

private:
    void PrintComponentDetails(Component* c) const {
        // Known component types
        if (auto* mr = dynamic_cast<MeshRenderer*>(c)) {
            const Material& m = mr->GetMaterial();
            std::cout << "    Mesh:      " << mr->GetMeshData().name
                      << " (" << mr->GetMeshData().VertexCount() << " verts)\n";
            std::cout << "    Material:  " << m.name << "\n";
            std::cout << "    Albedo:    (" << m.albedo.r << ", "
                      << m.albedo.g << ", " << m.albedo.b << ")\n";
            std::cout << "    Wireframe: " << (m.wireframe ? "Yes" : "No") << "\n";
        }
        else if (auto* cam = dynamic_cast<Camera*>(c)) {
            std::cout << "    FOV:    " << cam->GetFOV()  << "\n";
            std::cout << "    Near:   " << cam->GetNear() << "\n";
            std::cout << "    Far:    " << cam->GetFar()  << "\n";
        }
        else if (auto* lt = dynamic_cast<Light*>(c)) {
            const char* types[] = {"Directional","Point","Spot","Ambient"};
            int ti = (int)lt->GetType();
            std::cout << "    Type:      " << (ti < 4 ? types[ti] : "?") << "\n";
            std::cout << "    Intensity: " << lt->GetIntensity() << "\n";
            Vector3 ec = lt->GetEffectiveColor();
            std::cout << "    Color:     (" << ec.x << ", " << ec.y << ", " << ec.z << ")\n";
        }
    }
};

// -------------------------------------------------------
// EditorConsole – simple command-line interface to drive
// the engine interactively without a real window.
// -------------------------------------------------------
class EditorConsole {
public:
    EditorConsole(Scene* scene)
        : m_hierarchy(scene), m_running(true) {}

    void SetScene(Scene* scene) { m_hierarchy.SetScene(scene); }

    void PrintHelp() const {
        std::cout << "\nNovaEngine Console Commands:\n"
                  << "  list              - Show hierarchy\n"
                  << "  select <name>     - Select object by name\n"
                  << "  inspect           - Inspect selected object\n"
                  << "  create <name>     - Create new empty object\n"
                  << "  child <name>      - Create child under selected\n"
                  << "  delete            - Delete selected object\n"
                  << "  move <x> <y> <z>  - Move selected object\n"
                  << "  rotate <x> <y> <z>- Rotate selected object\n"
                  << "  help              - Show this help\n"
                  << "  quit              - Exit\n\n";
    }

    void ProcessCommand(const std::string& line) {
        if (line.empty()) return;
        std::vector<std::string> tokens = Split(line);
        if (tokens.empty()) return;
        const std::string& cmd = tokens[0];

        if (cmd == "list")    m_hierarchy.PrintHierarchy();
        else if (cmd == "help")  PrintHelp();
        else if (cmd == "quit" || cmd == "exit") m_running = false;
        else if (cmd == "inspect") {
            EditorInspector insp;
            insp.Inspect(m_hierarchy.GetSelected());
        }
        else if (cmd == "select" && tokens.size() >= 2) {
            m_hierarchy.SelectByName(tokens[1]);
            std::cout << "Selected: " << tokens[1] << "\n";
        }
        else if (cmd == "create") {
            std::string name = tokens.size() >= 2 ? tokens[1] : "GameObject";
            GameObject* go = m_hierarchy.CreateEmpty(name);
            if (go) std::cout << "Created: " << go->GetName() << " (ID " << go->GetID() << ")\n";
        }
        else if (cmd == "child") {
            std::string name = tokens.size() >= 2 ? tokens[1] : "Child";
            GameObject* go = m_hierarchy.CreateChild(name);
            if (go) std::cout << "Created child: " << go->GetName() << "\n";
        }
        else if (cmd == "delete") {
            m_hierarchy.DeleteSelected();
        }
        else if (cmd == "move" && tokens.size() >= 4) {
            GameObject* sel = m_hierarchy.GetSelected();
            if (!sel) { std::cout << "No object selected.\n"; return; }
            float x = std::stof(tokens[1]), y = std::stof(tokens[2]), z = std::stof(tokens[3]);
            sel->GetTransform()->SetPosition({x, y, z});
            std::cout << "Moved to (" << x << ", " << y << ", " << z << ")\n";
        }
        else if (cmd == "rotate" && tokens.size() >= 4) {
            GameObject* sel = m_hierarchy.GetSelected();
            if (!sel) { std::cout << "No object selected.\n"; return; }
            float x = std::stof(tokens[1]), y = std::stof(tokens[2]), z = std::stof(tokens[3]);
            sel->GetTransform()->SetEulerAngles({x, y, z});
            std::cout << "Rotated to (" << x << ", " << y << ", " << z << ") deg\n";
        }
        else {
            std::cout << "Unknown command: " << cmd << "  (type 'help')\n";
        }
    }

    bool IsRunning() const { return m_running; }

    // Run an interactive loop
    void RunLoop() {
        PrintHelp();
        std::string line;
        while (m_running) {
            std::cout << "nova> ";
            if (!std::getline(std::cin, line)) break;
            ProcessCommand(line);
        }
    }

private:
    std::vector<std::string> Split(const std::string& s) const {
        std::vector<std::string> tokens;
        std::string cur;
        for (char c : s) {
            if (c == ' ' || c == '\t') {
                if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
            } else {
                cur += c;
            }
        }
        if (!cur.empty()) tokens.push_back(cur);
        return tokens;
    }

    EditorHierarchy m_hierarchy;
    bool            m_running;
};

} // namespace Nova
