#pragma once
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
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
    // 窗口可见开关
    bool show_console_ = true;
    bool show_config_ = true;
    bool show_hierarchy_ = true;
    bool show_inspector_ = true;
    bool show_about_ = false;

    bool wants_quit_ = false;

    // FPS 曲线历史
    static constexpr int kFpsHistory = 300; // ~5s @60fps
    float fps_history_[kFpsHistory] = { 0 };
    int   fps_index_ = 0;

    // 棋盘格纹理
    unsigned int checker_tex_ = 0;
    int checker_w_ = 0, checker_h_ = 0;
    std::unordered_map<GameObject*, unsigned int> prev_tex_; // 恢复原贴图

    // 场景/选择指针
    std::vector<std::shared_ptr<GameObject>>* scene_ = nullptr;
    std::shared_ptr<GameObject>* selected_ = nullptr;

    // 绘制
    void drawMainMenu();
    void drawConsole();
    void drawConfig();
    void drawHierarchy();
    void drawInspector();

    // 工具
    void ensureChecker();
    void loadPrimitiveFromAssets(const std::string& name);
    std::string getAssetsPath();
};
