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
#include <imgui.h>


class AssetDatabase; // Forward declaration

class EditorWindows {
public:
    void init(SDL_Window* window, SDL_GLContext gl);
    void shutdown();
    void render(bool* isPlaying = nullptr, bool* isPaused = nullptr, bool* step = nullptr);
    void setScene(std::vector<std::shared_ptr<GameObject>>* scene,
        std::shared_ptr<GameObject>* selected);

    bool wantsQuit() const { return wants_quit_; }
    bool shouldShowAABBs() const { return show_aabbs_; }
    bool isFrustumCullingEnabled() const { return enable_frustum_culling_; } 
    bool shouldShowFrustum() const { return show_frustum_; }

    // Asset system integration
    void setAssetDatabase(AssetDatabase* db) { asset_database_ = db; }

    void drawAssets();
    void drawToolbar(bool& isPlaying, bool& isPaused, bool& step); // [NEW] Logic for Play/Stop
    void ensureChecker();


    struct ViewportBounds {
        float x, y; // Screen position of top-left corner
        float w, h; // Size of the viewport
        bool isHovered;
        bool isFocused;
    };

    // Camera Settings
    float cameraMoveSpeed = 10.0f;
    float cameraSensitivity = 0.1f;
    float cameraZoomSpeed = 2.0f;
    
    // Game View
    void setGameViewTexture(unsigned int texID, int w, int h) { gameTexID_ = texID; gameW_ = w; gameH_ = h; }
    void drawGameWindow(unsigned int texID, int w, int h, bool isPlaying);
    const ViewportBounds& getGameViewBounds() const { return gameViewBounds_; }

    // Scene View (Editor)
    void setSceneViewTexture(unsigned int texID, int w, int h) { sceneTexID_ = texID; sceneW_ = w; sceneH_ = h; }
    void drawSceneWindow(unsigned int texID, int w, int h);
    const ViewportBounds& getSceneViewBounds() const { return sceneViewBounds_; } // Accessor for Scene Bounds
    const ViewportBounds& getAssetsViewBounds() const;
    const std::string& getHoveredAssetFolder() const;


private:
    bool show_console_ = true;
    bool show_config_ = true;
    bool show_hierarchy_ = true;
    bool show_inspector_ = true;
    bool show_about_ = false;
    bool show_assets_ = true;
    bool wants_quit_ = false;
    bool show_aabbs_ = false;
    bool enable_frustum_culling_ = true;  
    bool show_frustum_ = false;

    static constexpr int kFpsHistory = 300;
    float fps_history_[kFpsHistory] = { 0 };
    int   fps_index_ = 0;
    unsigned int checker_tex_ = 0;
  int checker_w_ = 0, checker_h_ = 0;
    std::unordered_map<GameObject*, unsigned int> prev_tex_;
    std::vector<std::shared_ptr<GameObject>>* scene_ = nullptr;
    std::shared_ptr<GameObject>* selected_ = nullptr;

    AssetDatabase* asset_database_ = nullptr; // Asset database reference
    std::unordered_set<GameObject*> openNodes_;
    GameObject* pendingFocus_ = nullptr;
    bool preserve_world_ = true;


    GameObject* pendingDelete_ = nullptr;
    
    // Game View State
    unsigned int gameTexID_ = 0;
    int gameW_ = 0;
    int gameH_ = 0;
    ViewportBounds gameViewBounds_; // Underscore to match cpp

    // Scene View State
    unsigned int sceneTexID_ = 0;
    int sceneW_ = 0;
    int sceneH_ = 0;
    ViewportBounds sceneViewBounds_;
    ViewportBounds assetsViewBounds_;
    std::string hoveredAssetFolder_;
    GLuint previewTexID_ = 0;
    int previewW_ = 0, previewH_ = 0;
    ImGuiTextFilter assetsFilter_;

    // Assets window state
    std::unordered_set<std::string> expanded_asset_folders_;
    std::string selected_asset_;
    std::unordered_map<std::string, std::vector<std::string>> asset_folder_contents_;
    bool show_asset_refs_ = false;  // Show reference counts

    void drawMainMenu();
    void drawConsole();
    void drawConfig();
    void drawHierarchy();
    void drawHierarchyNode(GameObject*);
    void drawInspector();
    void drawAssetTree(const std::string& folderPath, int depth);
    void loadPrimitiveFromAssets(const std::string& name);
    std::string getAssetsPath();

    void deleteSelectedRecursive();
    void collectPostorder(GameObject* root, std::vector<GameObject*>& out);
    std::shared_ptr<GameObject> findShared(GameObject* raw);
    void removeFromScene(GameObject* raw);
    void reparent(GameObject* dragged, GameObject* target);
    void reorderSibling(GameObject* node, GameObject* parent, int newIndex);
    void reorderRoot(GameObject* node, int newIndex);

    void setSelection(const std::shared_ptr<GameObject>& go);
};
