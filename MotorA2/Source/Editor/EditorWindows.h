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

    // 让 main.cpp 把 SDL_Event 投喂进来（用于 ImGui 输入）
    // Feed SDL_Event to ImGui
    void processEvent(const SDL_Event& e);

    // 让 main.cpp 设置当前场景（所有对象由 shared_ptr 持有；树结构用 raw 指针链接）
    // Set current scene (objects owned by shared_ptr; tree uses raw pointers)
    void setScene(std::vector<std::shared_ptr<GameObject>>* scene);

private:
    // -------------------------
    // 状态 / State
    // -------------------------
    SDL_Window* window_ = nullptr;
    SDL_GLContext gl_ = nullptr;

    std::vector<std::shared_ptr<GameObject>>* scene_ = nullptr;

    bool show_demo_ = false;
    bool show_about_ = false;
    bool show_console_ = true;

    // 选择对象（shared_ptr 保持生命周期）
    // Selection (shared_ptr keeps lifetime)
    std::shared_ptr<GameObject> selected_ = nullptr;

    // Console buffer / 控制台文本缓存
    std::vector<std::string> console_;

    // -------------------------
    // UI drawing / UI 绘制
    // -------------------------
    void drawMainMenuBar();
    void drawHierarchy();
    void drawInspector(Camera* camera);
    void drawConsole();
    void drawAbout();

    // -------------------------
    // Scene helpers / 场景辅助
    // -------------------------
    void log(const std::string& s);

    // 获取 Assets 根目录（默认：<project>/Assets）
    // Get assets root path (default: <project>/Assets)
    std::string getAssetsPath();

    // 从 Assets/Street 里加载 FBX/DAE 等模型（你老师给的 Street）
    // Load model from Assets/Street (Street pack from teacher)
    void loadStreetAsset(const std::string& filename);

    // 递归画层级树（raw pointer only; ownership is in scene_）
    // Draw hierarchy recursively
    void drawHierarchyNode(GameObject* node);

    // 选择设置：同时维护 isSelected 标记（你的 GameObject 有这个字段）
    // Set selection: keep isSelected flag (your GameObject has it)
    void setSelection(const std::shared_ptr<GameObject>& go);

    // 查找 raw 指针对应的 shared_ptr（因为 children/parent 都是 raw）
    // Find shared_ptr from raw pointer (since tree uses raw pointers)
    std::shared_ptr<GameObject> findShared(GameObject* raw);

    // 收集后序遍历（用于安全删除：先删子再删父）
    // Collect postorder traversal (safe delete: children first)
    void collectPostorder(GameObject* root, std::vector<GameObject*>& out);

    // 从 scene_ 中移除一个 raw 对象（同时从父节点 children 中断开）
    // Remove raw object from scene_ (also detach from parent->children)
    void removeFromScene(GameObject* raw);

    // 删除当前选中对象（递归删子树）
    // Delete selected object (recursive)
    void deleteSelectedRecursive();

    // Reparent：把 dragged 挂到 target 下，同时保持世界变换不变
    // Reparent dragged under target, preserving world transform
    void reparent(GameObject* dragged, GameObject* target);

    // 重新排序：同一个 parent 下的 children 顺序调整
    // Reorder siblings under same parent
    void reorderSibling(GameObject* node, GameObject* parent, int newIndex);

    // 重新排序：根节点（scene_ vector 的顺序）调整
    // Reorder root objects (scene_ vector order)
    void reorderRoot(GameObject* node, int newIndex);
};
