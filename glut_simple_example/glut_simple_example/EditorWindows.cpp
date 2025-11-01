#include "EditorWindows.h"
#include "GameObject.h"     
#include "Transform.h"

#include <SDL3/SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"      
#include "imgui_impl_opengl3.h"   

#include <assimp/version.h>       
#include <IL/il.h>                

#include <cmath>
#include <cstring>
#include <string>


EditorWindows::EditorWindows() {}

EditorWindows::~EditorWindows() {}

void EditorWindows::init(SDL_Window* window, SDL_GLContext gl_context) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
 
    ImGui::StyleColorsDark();
    
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 130");
}

void EditorWindows::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void EditorWindows::renderMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {

        // File Menu
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Project")) {}
            if (ImGui::MenuItem("Open Project")) {}
            if (ImGui::MenuItem("Save Project")) {}
                ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {}
            ImGui::EndMenu();
        }

        // View Menu
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Console", NULL, &show_console);
            ImGui::MenuItem("Settings", NULL, &show_settings);
            ImGui::MenuItem("Hierarchy", NULL, &show_hierarchy);
            ImGui::MenuItem("Inspector", NULL, &show_inspector);
            ImGui::MenuItem("Geometry", NULL, &show_geometry_menu);
            ImGui::EndMenu();
        }

        // Help Menu
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Documentation")) {}
            if (ImGui::MenuItem("About")) {}
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void EditorWindows::renderConsoleWindow() {
    if (show_console) {
        ImGui::Begin("Console", &show_console);
        // Placeholder
        ImGui::Text("Console Window");
        ImGui::End();
    }
}

void EditorWindows::renderSettingsWindow() {
    if (show_settings) {
        ImGui::Begin("Settings", &show_settings);

        static uint64_t last = SDL_GetPerformanceCounter();
        static float fps = 0.0f;
        uint64_t now = SDL_GetPerformanceCounter();
        float dt = (float)((now - last) / (double)SDL_GetPerformanceFrequency());
        last = now;
        if (dt > 0) fps = 0.9f * fps + 0.1f * (1.0f / dt);

        ImGui::Text("FPS: %.1f", fps);
        ImGui::Text("OpenGL: %s", (const char*)glGetString(GL_VERSION));
        ImGui::Text("Assimp: %d.%d.%d",
            aiGetVersionMajor(), aiGetVersionMinor(), aiGetVersionPatch());

        ImGui::Text("DevIL: %d.%d.%d", IL_VERSION / 100, (IL_VERSION / 10) % 10, IL_VERSION % 10);

        ImGui::End();
    }
}

void EditorWindows::renderHierarchyWindow() {
    if (show_hierarchy) {
        ImGui::Begin("Hierarchy", &show_hierarchy);

        if (!sceneObjects) {
            ImGui::TextUnformatted("No scene bound.");
        }
        else {
            for (auto& go : *sceneObjects) {
                bool selectedNow = (selected && *selected && (*selected).get() == go.get());
                if (ImGui::Selectable(go->name.c_str(), selectedNow)) {
                    if (selected && *selected) (*selected)->isSelected = false;
                    if (selected) { *selected = go; go->isSelected = true; }
                }
            }
        }

        ImGui::End();
    }
}

void EditorWindows::renderInspectorWindow() {
    if (show_inspector) {
        ImGui::Begin("Inspector", &show_inspector);

        if (!(selected && *selected)) {
            ImGui::TextUnformatted("No object selected.");
            ImGui::End();
            return;
        }

        auto& go = *(*selected);
        ImGui::Text("Name: %s", go.name.c_str());
        ImGui::Separator();

        // Position
        auto& p = go.transform.pos();
        float v[3] = { (float)p.x, (float)p.y, (float)p.z };
        if (ImGui::DragFloat3("Position", v, 0.05f)) {
            p.x = v[0]; p.y = v[1]; p.z = v[2];
        }

        // Rotation
        static float deg[3] = { 0,0,0 };
        static float last[3] = { 0,0,0 };
        if (ImGui::DragFloat3("Rotate XYZ (deg)", deg, 0.2f)) {
            float dx = (deg[0] - last[0]) * 0.017453292f;
            float dy = (deg[1] - last[1]) * 0.017453292f;
            float dz = (deg[2] - last[2]) * 0.017453292f;
            if (fabsf(dx) > 1e-6f) go.transform.rotate(dx, { 1,0,0 });
            if (fabsf(dy) > 1e-6f) go.transform.rotate(dy, { 0,1,0 });
            if (fabsf(dz) > 1e-6f) go.transform.rotate(dz, { 0,0,1 });
            memcpy(last, deg, sizeof(deg));
        }

        ImGui::End();
    }
}

void EditorWindows::renderGeometryMenu() {
    if (show_geometry_menu) {
        ImGui::Begin("Basic Geometry", &show_geometry_menu);

        if (ImGui::TreeNode("Basic Shapes")) {
            ImGui::Selectable("Cube");
            ImGui::Selectable("Sphere");
            ImGui::Selectable("Cylinder");
            ImGui::Selectable("Plane");
            ImGui::TreePop();
        }

        ImGui::End();
    }
}

void EditorWindows::render() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    renderMainMenuBar();
    renderConsoleWindow();
    renderSettingsWindow();
    renderHierarchyWindow();
    renderInspectorWindow();
    renderGeometryMenu();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}