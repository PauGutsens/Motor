#pragma once
#include "imgui.h"
#include "types.h"
#include <SDL3/SDL.h>

struct SDL_Window;

class EditorWindows {
public:
    EditorWindows();
    ~EditorWindows();

    void init(SDL_Window* window, SDL_GLContext gl_context);
    void render();
    void shutdown();

private:
    void renderMainMenuBar();
    void renderConsoleWindow();
    void renderSettingsWindow();
    void renderHierarchyWindow();
    void renderInspectorWindow();
    void renderGeometryMenu();

    // Window visibility flags
    bool show_console = true;
    bool show_settings = true;
    bool show_hierarchy = true;
    bool show_inspector = true;
    bool show_geometry_menu = true;
};