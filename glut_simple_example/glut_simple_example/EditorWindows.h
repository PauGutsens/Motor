#pragma once
#include "imgui.h"
#include "types.h"
#include <SDL3/SDL.h>
#include <vector>
#include <memory>
class GameObject;

struct SDL_Window;

class EditorWindows {
public:
    EditorWindows();
    ~EditorWindows();

    void init(SDL_Window* window, SDL_GLContext gl_context);
    void render();
    void shutdown();
    void setScene(std::vector<std::shared_ptr<GameObject>>* objs,
        std::shared_ptr<GameObject>* selectedPtr) {
        sceneObjects = objs;
        selected = selectedPtr;
    }

private:
    void renderMainMenuBar();
    void renderConsoleWindow();
    void renderSettingsWindow();
    void renderHierarchyWindow();
    void renderInspectorWindow();
    void renderGeometryMenu();

    std::vector<std::shared_ptr<GameObject>>* sceneObjects = nullptr;
    std::shared_ptr<GameObject>* selected = nullptr;

    // Window visibility flags
    bool show_console = true;
    bool show_settings = true;
    bool show_hierarchy = true;
    bool show_inspector = true;
    bool show_geometry_menu = true;
};