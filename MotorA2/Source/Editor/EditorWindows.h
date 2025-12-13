#pragma once
// EditorWindows.h
// 中英双语注释 / Chinese-English bilingual comments

#include <vector>
#include <memory>
#include <string>

#include <SDL3/SDL.h>

#include "types.h"
#include "Logger.h"
#include "GameObject.h"

class Camera; // Forward declaration / 前置声明

class EditorWindows {
public:
    // 初始化 ImGui + 绑定 SDL3/OpenGL
    // Init ImGui + SDL3/OpenGL backend
    void init(SDL_Window* window, SDL_GLContext gl);

    // 释放 ImGui / Destroy ImGui
    void shutdown();

    // 每帧开始：NewFrame
    // Per-frame begin: NewFrame
    void newFrame();

    // 渲染所有窗口（菜单、层级、Inspector、Console…）
    // Render all editor windows (menu, hierarchy, inspector, console...)
    void render(Camera* camera);

    // 兼容 main.cpp：传引用也可以
    void render(Camera& camera) { render(&camera); }

    // 让 main.cpp 把 SDL_Event 投喂进来（用于 ImGui 输入）
    // Feed SDL_Event to ImGui
    void processEvent(const SDL_Event& e);

    // Set current scene (objects owned by shared_ptr; tree uses raw pointers)
    void setScene(std::vector<std::shared_ptr<GameObject>>* scene);

    // 兼容 main.cpp：同时把 selected 引用交给编辑器（可选）
    // Compatibility: also pass selected reference (optional)
    void setScene(std::vector<std::shared_ptr<GameObject>>* scene, std::shared_ptr<GameObject>& selected);

    // 让 main.cpp 设置主相机指针
    // Set main camera pointer
    void setMainCamera(Camera* cam);

    // 主循环退出标记（main.cpp 用）
    // Quit flag for main loop
    bool wantsQuit() const;

private:
    // -------------------------
    // 状态 / State
    // -------------------------
    SDL_Window* window_ = nullptr;
    SDL_GLContext gl_ = nullptr;

    std::vector<std::shared_ptr<GameObject>>* scene_ = nullptr;

    // main.cpp 传进来的“当前选中对象”引用（可选）
    // Optional external selected reference
    std::shared_ptr<GameObject>* selectedRef_ = nullptr;

    Camera* mainCam_ = nullptr;
    bool quit_ = false;

    bool show_demo_ = false;
    bool show_about_ = false;
    bool show_console_ = true;

    // 选择对象（shared_ptr 保持生命周期）
    std::shared_ptr<GameObject> selected_ = nullptr;

    // Console buffer
    std::vector<std::string> console_;

    // -------------------------
    // UI drawing
    // -------------------------
    void drawMainMenuBar();
    void drawHierarchy();
    void drawInspector(Camera* camera);
    void drawConsole();
    void drawAbout();

    // -------------------------
    // Scene helpers
    // -------------------------
    void log(const std::string& s);

    std::string getAssetsPath();
    void loadStreetAsset(const std::string& filename);

    void drawHierarchyNode(GameObject* node);
    void setSelection(const std::shared_ptr<GameObject>& go);
    std::shared_ptr<GameObject> findShared(GameObject* raw);

    void collectPostorder(GameObject* root, std::vector<GameObject*>& out);
    void removeFromScene(GameObject* raw);
    void deleteSelectedRecursive();

    void reparent(GameObject* dragged, GameObject* target);
    void reorderSibling(GameObject* node, GameObject* parent, int newIndex);
    void reorderRoot(GameObject* node, int newIndex);
};
