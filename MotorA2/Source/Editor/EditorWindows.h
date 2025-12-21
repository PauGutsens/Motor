#pragma once

#include <vector>
#include <memory>
#include <string>

#include <SDL3/SDL.h>
#include <GL/glew.h>

#include "types.h"
#include "Logger.h"
#include "GameObject.h"

class Camera;

class EditorWindows {
public:
    void init(SDL_Window* window, SDL_GLContext gl);
    void shutdown();

    void processEvent(const SDL_Event& e);

    void newFrame();

    // Build UI windows (does not call ImGui::Render)
    void drawUI(Camera* camera);

    // Submit ImGui draw data to default framebuffer
    void renderImGui();

    void setScene(std::vector<std::shared_ptr<GameObject>>* scene);

    // Viewport state
    bool isViewportHovered() const { return viewportHovered_; }
    bool isViewportFocused() const { return viewportFocused_; }
    int  viewportWidth()  const { return viewportW_; }
    int  viewportHeight() const { return viewportH_; }

    // Play/Pause state (foundation)
    bool isPlaying() const { return playState_ != PlayState::Edit; }
    bool isPaused()  const { return playState_ == PlayState::Paused; }

    // Viewport render target
    bool beginViewportRender();
    void endViewportRender();
    GLuint viewportTexture() const { return viewportColorTex_; }

private:
    SDL_Window* window_ = nullptr;
    SDL_GLContext gl_ = nullptr;

    std::vector<std::shared_ptr<GameObject>>* scene_ = nullptr;

    bool show_demo_ = false;
    bool show_about_ = false;
    bool show_console_ = true;

    std::shared_ptr<GameObject> selected_ = nullptr;
    std::vector<std::string> console_;

    // -------- Scene Save/Load (custom file) --------
    std::string currentScenePath_;
    bool openSaveScenePopup_ = false;
    bool openLoadScenePopup_ = false;
    char scenePathBuf_[512] = {};

    // -------- Play/Pause/Stop (foundation) --------
    enum class PlayState { Edit, Playing, Paused };
    PlayState playState_ = PlayState::Edit;
    std::string playBackup_; // serialized snapshot before entering Play

    // Viewport
    bool viewportHovered_ = false;
    bool viewportFocused_ = false;
    int  viewportW_ = 0;
    int  viewportH_ = 0;

    GLuint viewportFBO_ = 0;
    GLuint viewportColorTex_ = 0;
    GLuint viewportDepthRBO_ = 0;

    void ensureViewportRT(int w, int h);
    void destroyViewportRT();

    // UI
    void drawMainMenuBar();
    void drawViewportWindow(Camera* camera, float x, float y, float w, float h);

    void drawHierarchy(Camera* camera, float x, float y, float w, float h);
    void drawInspector(Camera* camera, float x, float y, float w, float h);
    void drawConsole(float x, float y, float w, float h);
    void drawAbout();

    // Scene IO popups
    std::string defaultScenePath();
    void drawSceneIOPopups();
    void doSaveScene(const std::string& path);
    void doLoadScene(const std::string& path);

    // Play control handlers
    void onPlayPressed();
    void onPausePressed();
    void onStopPressed();

    // Helpers
    void log(const std::string& s);
    std::string getAssetsPath();
    void loadStreetAsset(const std::string& filename, Camera* camera = nullptr);

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
