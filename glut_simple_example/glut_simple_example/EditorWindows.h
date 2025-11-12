#pragma once
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "types.h"
#include "GameObject.h"
#include "Logger.h"
#include <SDL3/SDL.h>

class EditorWindows {
public:
    void init(SDL_Window* window, SDL_GLContext gl);
    void shutdown();
    void render();
    void setScene(std::vector<std::shared_ptr<GameObject>>* scene,
        std::shared_ptr<GameObject>* selected);
    bool wantsQuit() const { return wants_quit_; }

private:
    bool show_console_ = true;
    bool show_config_ = true;
    bool show_hierarchy_ = true;
    bool show_inspector_ = true;
    bool show_about_ = false;
    bool wants_quit_ = false;

    static constexpr int kFpsHistory = 300;
    float fps_history_[kFpsHistory] = { 0 };
    int   fps_index_ = 0;
    unsigned int checker_tex_ = 0;
    int checker_w_ = 0, checker_h_ = 0;
    std::unordered_map<GameObject*, unsigned int> prev_tex_;
    std::vector<std::shared_ptr<GameObject>>* scene_ = nullptr;
    std::shared_ptr<GameObject>* selected_ = nullptr;
    std::unordered_set<GameObject*> openNodes_;
    GameObject* pendingFocus_ = nullptr;

    void drawMainMenu();
    void drawConsole();
    void drawConfig();
    void drawHierarchy();
    void drawHierarchyNode(GameObject*);
    void drawInspector();
    void ensureChecker();
    void loadPrimitiveFromAssets(const std::string& name);
    std::string getAssetsPath();

    void setSelection(const std::shared_ptr<GameObject>& go);
};
