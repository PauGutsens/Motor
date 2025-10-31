#include "EditorWindows.h"
#include "imgui.h"
#include "backends/imgui/imgui_impl_sdl3.h"
#include "backends/imgui/imgui_impl_opengl3.h"
#include <SDL3/SDL.h>
#include <GL/glew.h>

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
        // Placeholder
        ImGui::Text("Settings Window");
        ImGui::End();
    }
}

void EditorWindows::renderHierarchyWindow() {
    if (show_hierarchy) {
        ImGui::Begin("Hierarchy", &show_hierarchy);
        // Placeholder
        ImGui::Text("Hierarchy Window");
        ImGui::End();
    }
}

void EditorWindows::renderInspectorWindow() {
    if (show_inspector) {
        ImGui::Begin("Inspector", &show_inspector);
        // Placeholder
        ImGui::Text("Inspector Window");
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