#include "EditorWindows.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

#include <SDL3/SDL.h>
#include <GL/glew.h>
//
//#include <filesystem>
//#include <algorithm>
//
//#include "Camera.h"
#include <filesystem>
#include <algorithm>
#include <cstdio>

#include "Camera.h"
#include "SceneIO.h"
#include "AABB.h"

#include "ModelLoader.h"
#include "ImGuizmo.h"
#include <glm/gtc/type_ptr.hpp>

namespace fs = std::filesystem;

static void glTextureSize(GLuint tex, int& w, int& h) {
    w = h = 0;
    if (!tex) return;
    glBindTexture(GL_TEXTURE_2D, tex);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void EditorWindows::init(SDL_Window* window, SDL_GLContext gl) {
    window_ = window;
    gl_ = gl;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // ⚠️ 你的 ImGui 没有 Docking 分支，所以这里不启用 Docking

    // fonts
    io.Fonts->AddFontDefault();
    {
        ImFontConfig cfg;
        cfg.MergeMode = true;
        cfg.PixelSnapH = true;
        static const ImWchar ranges[] = {
            0x0020, 0x00FF,
            0x4E00, 0x9FA5,
            0
        };

        fs::path fontPath = fs::current_path() / "Assets" / "Fonts" / "NotoSansSC-Regular.otf";
        if (fs::exists(fontPath)) {
            io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), 16.0f, &cfg, ranges);
        }
    }

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForOpenGL(window_, gl_);
    ImGui_ImplOpenGL3_Init("#version 330");

    log("Editor initialized");
    currentScenePath_ = defaultScenePath();
    std::snprintf(scenePathBuf_, sizeof(scenePathBuf_), "%s", currentScenePath_.c_str());
}

void EditorWindows::shutdown() {
    destroyViewportRT();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    window_ = nullptr;
    gl_ = nullptr;
    scene_ = nullptr;
    selected_.reset();
}

void EditorWindows::processEvent(const SDL_Event& e) {
    ImGui_ImplSDL3_ProcessEvent(&e);
}

void EditorWindows::newFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void EditorWindows::renderImGui() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorWindows::setScene(std::vector<std::shared_ptr<GameObject>>* scene) {
    scene_ = scene;
}

void EditorWindows::log(const std::string& s) {
    console_.push_back(s);
    LOG_INFO(s);
}

std::string EditorWindows::getAssetsPath() {
    fs::path cwd = fs::current_path();
    for (int i = 0; i < 6; ++i) {
        fs::path candidate = cwd / "Assets";
        if (fs::exists(candidate) && fs::is_directory(candidate)) {
            return candidate.string();
        }
        cwd = cwd.parent_path();
        if (cwd.empty()) break;
    }
    return (fs::current_path() / "Assets").string();
}

void EditorWindows::loadStreetAsset(const std::string& filename, Camera* camera) {
    if (!scene_) { log("Scene is null setScene()"); return; }

    fs::path p = fs::path(getAssetsPath()) / "Street" / filename;
    if (!fs::exists(p)) { log(std::string("Asset not found: ") + p.string()); return; }

    auto meshes = ModelLoader::loadModel(p.string());
    if (meshes.empty()) { log(std::string("Failed to load model: ") + p.string()); return; }

    std::shared_ptr<GameObject> firstGo = nullptr;
    std::string baseName = p.stem().string();

    for (size_t i = 0; i < meshes.size(); ++i) {
        auto& mesh = meshes[i];
        if (!mesh) continue;

        std::string goName = (meshes.size() == 1) ? baseName : (baseName + "_" + std::to_string(i));
        auto go = std::make_shared<GameObject>(goName);
        go->setMesh(mesh);

        // Tu rotación para levantar la casa
        go->transform.rotateEulerDeltaDeg(vec3(-90.0, 0.0, 0.0));

        fs::path assetsRoot = fs::path(getAssetsPath());
        std::error_code ec;
        fs::path rel = fs::relative(p, assetsRoot, ec);
        go->sourceModel = ec ? p.filename().string() : rel.generic_string();
        go->sourceMeshIndex = (int)i;
        go->parent = nullptr;
        go->children.clear();

        scene_->push_back(go);
        if (!firstGo) firstGo = go;
    }

    if (firstGo) setSelection(firstGo);
    log(std::string("Loaded: ") + p.string());

    if (camera)
    {
        // 1. Definir dónde está la cámara y a dónde mira
        vec3 eye = vec3(0.0f, 20.0f, 60.0f);
        vec3 center = vec3(0.0f, 0.0f, 0.0f);
        vec3 up = vec3(0.0f, 1.0f, 0.0f);

        // 2. Crear una matriz de vista manualmente usando GLM
        glm::mat4 view = glm::lookAt(eye, center, up);

        // 3. Ajustar variables de órbita (Ahora funciona porque las hiciste PUBLICAS en el Paso 1)
        camera->_orbitTarget = center;
        camera->_orbitDistance = glm::length(eye - center);

        // 4. Aplicar todo a la cámara
        // Esto coloca la cámara, calcula los ángulos y actualiza la vista
        camera->setFromViewMatrix(view);
    }
}

//void EditorWindows::drawMainMenuBar() {
//    if (!ImGui::BeginMainMenuBar()) return;
//
//    if (ImGui::BeginMenu("File")) {
//        if (ImGui::MenuItem("Load Street environment (FBX)")) loadStreetAsset("Street environment_V01.FBX");
//        if (ImGui::MenuItem("Load street2 (FBX)"))             loadStreetAsset("street2.FBX");
//        if (ImGui::MenuItem("Load scene (DAE)"))               loadStreetAsset("scene.DAE");
//        ImGui::EndMenu();
//    }
//
//    if (ImGui::BeginMenu("Window")) {
//        ImGui::MenuItem("Console", nullptr, &show_console_);
//        ImGui::MenuItem("ImGui Demo", nullptr, &show_demo_);
//        ImGui::MenuItem("About", nullptr, &show_about_);
//        ImGui::EndMenu();
//    }
//
//    ImGui::EndMainMenuBar();
//}
void EditorWindows::drawMainMenuBar() {
    if (!ImGui::BeginMainMenuBar()) return;

    // --- Play controls (foundation) ---
    {
        const bool inEdit = (playState_ == PlayState::Edit);
        const bool inPlay = (playState_ == PlayState::Playing);
        const bool inPaused = (playState_ == PlayState::Paused);

        if (ImGui::Button("Play"))   onPlayPressed();
        ImGui::SameLine();
        if (ImGui::Button(inPaused ? "Resume" : "Pause")) onPausePressed();
        ImGui::SameLine();
        if (ImGui::Button("Stop"))   onStopPressed();

        ImGui::SameLine();
        ImGui::TextUnformatted(inEdit ? "Mode: Edit" : (inPlay ? "Mode: Play" : "Mode: Paused"));

        /* ImGui::SameLine();
         ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
         ImGui::SameLine();*/
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

    }

    // --- Menus ---
    if (ImGui::BeginMenu("File")) {
        // Scene file (custom format)
        if (ImGui::MenuItem("New Scene")) {
            if (scene_) scene_->clear();
            setSelection(nullptr);
            currentScenePath_ = defaultScenePath();
            std::snprintf(scenePathBuf_, sizeof(scenePathBuf_), "%s", currentScenePath_.c_str());
            log("New scene");
        }

        if (ImGui::MenuItem("Save Scene (Ctrl+S)")) {
            if (!currentScenePath_.empty()) doSaveScene(currentScenePath_);
            else openSaveScenePopup_ = true;
        }
        if (ImGui::MenuItem("Save Scene As...")) openSaveScenePopup_ = true;

        if (ImGui::MenuItem("Load Scene...")) openLoadScenePopup_ = true;

        ImGui::Separator();

        // Quick asset import (existing)
        if (ImGui::MenuItem("Load Street environment (FBX)")) loadStreetAsset("Street environment_V01.FBX");
        if (ImGui::MenuItem("Load street2 (FBX)"))             loadStreetAsset("street2.FBX");
        if (ImGui::MenuItem("Load scene (DAE)"))               loadStreetAsset("scene.DAE");

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Window")) {
        ImGui::MenuItem("Console", nullptr, &show_console_);
        ImGui::MenuItem("ImGui Demo", nullptr, &show_demo_);
        ImGui::MenuItem("About", nullptr, &show_about_);
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

// ---------------- Viewport RT ----------------
void EditorWindows::destroyViewportRT() {
    if (viewportDepthRBO_) { glDeleteRenderbuffers(1, &viewportDepthRBO_); viewportDepthRBO_ = 0; }
    if (viewportColorTex_) { glDeleteTextures(1, &viewportColorTex_); viewportColorTex_ = 0; }
    if (viewportFBO_) { glDeleteFramebuffers(1, &viewportFBO_); viewportFBO_ = 0; }
}

void EditorWindows::ensureViewportRT(int w, int h) {
    if (w <= 0 || h <= 0) return;

    if (viewportColorTex_ != 0) {
        int tw = 0, th = 0;
        glTextureSize(viewportColorTex_, tw, th);
        if (tw == w && th == h) return;
    }

    destroyViewportRT();

    glGenFramebuffers(1, &viewportFBO_);
    glBindFramebuffer(GL_FRAMEBUFFER, viewportFBO_);

    glGenTextures(1, &viewportColorTex_);
    glBindTexture(GL_TEXTURE_2D, viewportColorTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, viewportColorTex_, 0);

    glGenRenderbuffers(1, &viewportDepthRBO_);
    glBindRenderbuffer(GL_RENDERBUFFER, viewportDepthRBO_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, viewportDepthRBO_);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        destroyViewportRT();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        log("Viewport FBO incomplete");
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool EditorWindows::beginViewportRender() {
    if (viewportW_ <= 0 || viewportH_ <= 0) return false;
    ensureViewportRT(viewportW_, viewportH_);
    if (!viewportFBO_) return false;

    glBindFramebuffer(GL_FRAMEBUFFER, viewportFBO_);
    glViewport(0, 0, viewportW_, viewportH_);
    return true;
}

void EditorWindows::endViewportRender() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ---------------- Layout (engine-like fixed panels) ----------------
void EditorWindows::drawUI(Camera* camera) {
    drawMainMenuBar();
    drawSceneIOPopups();
    ImGuizmo::BeginFrame();

    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImVec2 wp = vp->WorkPos;   // 不含主菜单栏
    ImVec2 ws = vp->WorkSize;

    float leftW = 300.0f;
    float rightW = 380.0f;
    float bottomH = show_console_ ? 220.0f : 0.0f;

    // clamp
    leftW = std::min(leftW, ws.x * 0.45f);
    rightW = std::min(rightW, ws.x * 0.45f);
    bottomH = std::min(bottomH, ws.y * 0.60f);

    float centerX = wp.x + leftW;
    float centerY = wp.y;
    float centerW = ws.x - leftW - rightW;
    float centerH = ws.y - bottomH;

    // Panels
    drawHierarchy(camera, wp.x, wp.y, leftW, centerH);
    drawInspector(camera, wp.x + ws.x - rightW, wp.y, rightW, centerH);
    if (show_console_) drawConsole(wp.x, wp.y + centerH, ws.x, bottomH);

    // Viewport uses the center area
   /* drawViewportWindow(centerX, centerY, centerW, centerH);*/
    drawViewportWindow(camera, centerX, centerY, centerW, centerH);

    if (show_demo_) ImGui::ShowDemoWindow(&show_demo_);
    if (show_about_) drawAbout();
}

static ImGuiWindowFlags PanelFlags() {
    return ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse;
}
static Ray MakePickRayFromViewport(Camera* camera, float mouseX, float mouseY,
    float rectX, float rectY, float rectW, float rectH)
{
    // mouseX/mouseY 是屏幕坐标（ImGui::GetMousePos）
    // rectX/Y/W/H 是 Viewport 图像区域的屏幕矩形

    // 转成 Viewport 内部像素坐标
    double mx = (double)(mouseX - rectX);
    double my = (double)(mouseY - rectY);

    // 归一化到 NDC：x [-1,1], y [-1,1]（注意 y 翻转）
    double x = (2.0 * mx) / (double)rectW - 1.0;
    double y = 1.0 - (2.0 * my) / (double)rectH;

    mat4 V = camera->view();
    mat4 P = camera->projection();
    mat4 invVP = glm::inverse(P * V);

    vec4 nearH = invVP * vec4(x, y, -1.0, 1.0);
    vec4 farH = invVP * vec4(x, y, 1.0, 1.0);

    vec3 nearP = vec3(nearH) / nearH.w;
    vec3 farP = vec3(farH) / farH.w;

    vec3 dir = glm::normalize(farP - nearP);
    return Ray(nearP, dir);
}

static void CollectAllNodes(GameObject* root, std::vector<GameObject*>& out)
{
    if (!root) return;
    out.push_back(root);
    for (auto* c : root->children) CollectAllNodes(c, out);
}

/*void EditorWindows::drawViewportWindow(float x, float y, float w, float h)*/
void EditorWindows::drawViewportWindow(Camera* camera, float x, float y, float w, float h)
{
    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);

    if (!ImGui::Begin("Viewport", nullptr, PanelFlags())) { ImGui::End(); return; }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    viewportW_ = (int)std::max(1.0f, avail.x);
    viewportH_ = (int)std::max(1.0f, avail.y);

    viewportHovered_ = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    viewportFocused_ = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    if (viewportColorTex_ != 0) {
        ImGui::Image((ImTextureID)(intptr_t)viewportColorTex_,
            ImVec2((float)viewportW_, (float)viewportH_),
            ImVec2(0, 1), ImVec2(1, 0));
    }
    else {
        ImGui::TextUnformatted("Viewport RT not ready...");
    }
    // ---------- Viewport rect (screen space) ----------
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 crMin = ImGui::GetWindowContentRegionMin();
    float rectX = winPos.x + crMin.x;
    float rectY = winPos.y + crMin.y;
    float rectW = (float)viewportW_;
    float rectH = (float)viewportH_;

    // ---------- ViewCube (always available) ----------
    if (camera)
    {
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(rectX, rectY, rectW, rectH);

        glm::mat4 viewF = glm::mat4(camera->view());
        const float viewSize = 96.0f;
        const float pad = 4.0f;

        ImGuizmo::ViewManipulate(
            glm::value_ptr(viewF),
            8.0f,
            ImVec2(rectX + rectW - viewSize - pad, rectY + pad),
            ImVec2(viewSize, viewSize),
            0x00000000
        );

        camera->setFromViewMatrix(viewF);
    }

    // --------------------------
    // Viewport Click Picking
    // --------------------------
    bool imageHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    if (camera && scene_ && imageHovered)
    {
        // 避免与 Gizmo 冲突：鼠标在 Gizmo 上/正在拖 Gizmo 时不做点选
        bool gizmoBusy = ImGuizmo::IsOver() || ImGuizmo::IsUsing();

        // 左键单击（不按 Alt，避免与相机轨道操作冲突）
        if (!gizmoBusy && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsKeyDown(ImGuiKey_LeftAlt) && !ImGui::IsKeyDown(ImGuiKey_RightAlt))
        {
            ImVec2 mouse = ImGui::GetMousePos();

            // 点在 ViewCube 区域：跳过 picking（让 ViewManipulate 处理）
            const float viewSize = 96.0f;
            const float pad = 4.0f;
            float cubeX0 = rectX + rectW - viewSize - pad;
            float cubeY0 = rectY + pad;
            float cubeX1 = cubeX0 + viewSize;
            float cubeY1 = cubeY0 + viewSize;

            bool skipPicking =
                (mouse.x >= cubeX0 && mouse.x <= cubeX1 &&
                    mouse.y >= cubeY0 && mouse.y <= cubeY1);

            if (!skipPicking)
            {

                // 生成拾取射线
                Ray ray = MakePickRayFromViewport(camera, mouse.x, mouse.y, rectX, rectY, rectW, rectH);

                // 收集所有可选节点（root + children）
                std::vector<GameObject*> nodes;
                nodes.reserve(scene_->size() * 2);

                for (auto& sp : *scene_)
                {
                    if (!sp) continue;
                    // scene_ 里可能包含所有节点，也可能只有 roots
                    // 为了兼容两种：只从 root（parent==nullptr）递归收集一次
                    if (sp->parent == nullptr)
                        CollectAllNodes(sp.get(), nodes);
                }

                // 找最近命中
                GameObject* best = nullptr;
                double bestT = 1e100;

                for (GameObject* go : nodes)
                {
                    if (!go) continue;

                    AABB box = go->computeWorldAABB();
                    if (!box.isValid()) continue;

                    double t = 0.0;
                    if (ray.intersectsAABB(box, t))
                    {
                        if (t > 0.0 && t < bestT)
                        {
                            bestT = t;
                            best = go;
                        }
                    }
                }

                if (best)
                    setSelection(findShared(best));
                else
                    setSelection(nullptr); // 点空白：取消选择

            }
        }
    }

    // ======================
// Gizmo (ImGuizmo) begin
// ======================
    if (camera && selected_ && viewportColorTex_ != 0) {

        // W/E/R 切换模式（仅在 Viewport 有焦点时）
        static ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
        static ImGuizmo::MODE mode = ImGuizmo::LOCAL;

        if (viewportFocused_) {
            if (ImGui::IsKeyPressed(ImGuiKey_W)) op = ImGuizmo::TRANSLATE;
            if (ImGui::IsKeyPressed(ImGuiKey_E)) op = ImGuizmo::ROTATE;
            if (ImGui::IsKeyPressed(ImGuiKey_R)) op = ImGuizmo::SCALE;

            // 可选：按 Q 切换世界/本地坐标
            if (ImGui::IsKeyPressed(ImGuiKey_Q)) {
                mode = (mode == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
            }
            // Frame Selected (F)
            if (ImGui::IsKeyPressed(ImGuiKey_F) && camera && selected_) {

                auto isValid = [](const AABB& b) {
                    return (b.min.x <= b.max.x) && (b.min.y <= b.max.y) && (b.min.z <= b.max.z);
                    };

                AABB box = selected_->computeWorldAABB();
                if (isValid(box)) {

                    vec3 center = box.center();
                    vec3 extent = box.max - box.min;
                    double radius = glm::length(extent) * 0.5;

                    // 防止半径为 0（点/空物体）
                    radius = std::max(radius, 0.01);

                    //// 1) 设置 orbit pivot
                    //camera->_orbitTarget = center;

                    //// 2) 用 FOV 算合适距离（确保能看到整个物体）
                    //double fovRad = camera->fov * (3.14159265358979323846 / 180.0);
                    //camera->_orbitDistance = radius / std::tan(fovRad * 0.5);

                    //// 3) 立刻把相机放到 pivot 前方（避免等一帧）
                    //vec3 fwd = camera->forward();
                    //camera->transform.pos() = camera->_orbitTarget - fwd * camera->_orbitDistance;
                    camera->frameTo(center, radius);

                }
            }

        }

        // 让相机投影匹配当前 viewport
        camera->aspect = (viewportH_ > 0) ? (double)viewportW_ / (double)viewportH_ : camera->aspect;

        // 计算 Viewport 的屏幕 rect（ImGuizmo 用屏幕坐标）
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 crMin = ImGui::GetWindowContentRegionMin();
        float rectX = winPos.x + crMin.x;
        float rectY = winPos.y + crMin.y;
        float rectW = (float)viewportW_;
        float rectH = (float)viewportH_;

        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(rectX, rectY, rectW, rectH);

        // ImGuizmo 需要 float 矩阵，你的 types.h 用的是 dmat4（double）
        glm::mat4 viewF = glm::mat4(camera->view());
        glm::mat4 projF = glm::mat4(camera->projection());

        // 选中物体的世界矩阵
        mat4 worldD = computeWorldMatrix(selected_.get());
        glm::mat4 worldF = glm::mat4(worldD);

        // Manipulate 会直接修改 worldF
        ImGuizmo::Manipulate(
            glm::value_ptr(viewF),
            glm::value_ptr(projF),
            op,
            mode,
            glm::value_ptr(worldF)
        );
        // ★ 这就是你刚才“找不到的步骤 3”
        // 当正在拖 gizmo 时，把新 world 写回 local（保持父子层级正确）
        if (ImGuizmo::IsUsing()) {
            mat4 newWorldD = mat4(worldF); // float->double
            setLocalFromWorld(selected_.get(), newWorldD, selected_->parent);
        }
    }
    // ====================
    // Gizmo (ImGuizmo) end
    // ====================

    ImGui::End();
}

void EditorWindows::drawHierarchy(Camera* camera, float x, float y, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
    if (!ImGui::Begin("Hierarchy", nullptr, PanelFlags())) { ImGui::End(); return; }

    if (!scene_) { ImGui::TextUnformatted("Scene is null"); ImGui::End(); return; }

    if (ImGui::Button("Delete Selected")) deleteSelectedRecursive();
    ImGui::SameLine();
    if (ImGui::Button("Load Street"))
    {
        // Pásale 'camera' (que te llega por argumento a drawHierarchy)
        loadStreetAsset("Street environment_V01.FBX", camera);
    }
    ImGui::Separator();

    for (auto& sp : *scene_) {
        if (!sp) continue;
        if (sp->parent != nullptr) continue;
        drawHierarchyNode(sp.get());
    }

    ImGui::End();
}

void EditorWindows::drawConsole(float x, float y, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
    if (!ImGui::Begin("Console", &show_console_, PanelFlags())) { ImGui::End(); return; }

    static bool auto_scroll = true;
    ImGui::Checkbox("Auto-scroll", &auto_scroll);
    ImGui::Separator();

    ImGui::BeginChild("console_scroller", ImVec2(0, 0), false);
    for (const auto& line : console_) ImGui::TextUnformatted(line.c_str());
    if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();

    ImGui::End();
}

void EditorWindows::drawAbout() {
    ImGui::SetNextWindowSize(ImVec2(420, 220), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("About", &show_about_)) { ImGui::End(); return; }
    ImGui::Text("MotorA2 - Editor");
    ImGui::Separator();
    ImGui::BulletText("RMB drag: free look");
    ImGui::BulletText("Wheel: dolly");
    ImGui::BulletText("MMB or Shift+RMB: pan");
    ImGui::BulletText("RMB + WASDQE: fly");
    ImGui::End();
}

// --------- Scene operations (保持你原来的逻辑) ----------
void EditorWindows::setSelection(const std::shared_ptr<GameObject>& go) {
    if (selected_) selected_->isSelected = false;
    selected_ = go;
    if (selected_) selected_->isSelected = true;
}

std::shared_ptr<GameObject> EditorWindows::findShared(GameObject* raw) {
    if (!scene_ || !raw) return nullptr;
    for (auto& sp : *scene_) if (sp.get() == raw) return sp;
    return nullptr;
}

void EditorWindows::collectPostorder(GameObject* root, std::vector<GameObject*>& out) {
    if (!root) return;
    for (GameObject* c : root->children) collectPostorder(c, out);
    out.push_back(root);
}

void EditorWindows::removeFromScene(GameObject* raw) {
    if (!scene_ || !raw) return;
    if (raw->parent) raw->parent->removeChild(raw);
    scene_->erase(std::remove_if(scene_->begin(), scene_->end(),
        [raw](const std::shared_ptr<GameObject>& sp) { return sp.get() == raw; }), scene_->end());
}

void EditorWindows::deleteSelectedRecursive() {
    if (!scene_ || !selected_) return;
    std::vector<GameObject*> post;
    collectPostorder(selected_.get(), post);
    setSelection(nullptr);
    for (GameObject* n : post) removeFromScene(n);
    log("Deleted selection subtree");
}

void EditorWindows::reparent(GameObject* dragged, GameObject* target) {
    if (!dragged || !target) return;
    if (dragged == target) return;
    if (target->isDescendantOf(dragged)) return;

    mat4 M_world = computeWorldMatrix(dragged);
    if (dragged->parent) dragged->parent->removeChild(dragged);
    target->addChild(dragged);
    setLocalFromWorld(dragged, M_world, target);
}

void EditorWindows::reorderSibling(GameObject* node, GameObject* parent, int newIndex) {
    if (!node || !parent) return;
    auto& v = parent->children;
    int cur = -1;
    for (int i = 0; i < (int)v.size(); ++i) if (v[i] == node) { cur = i; break; }
    if (cur < 0) return;
    newIndex = std::max(0, std::min(newIndex, (int)v.size() - 1));
    if (newIndex == cur) return;
    GameObject* tmp = v[cur];
    v.erase(v.begin() + cur);
    v.insert(v.begin() + newIndex, tmp);
}

void EditorWindows::reorderRoot(GameObject* node, int newIndex) {
    if (!scene_ || !node) return;
    int cur = -1;
    for (int i = 0; i < (int)scene_->size(); ++i) if ((*scene_)[i].get() == node) { cur = i; break; }
    if (cur < 0) return;
    newIndex = std::max(0, std::min(newIndex, (int)scene_->size() - 1));
    if (newIndex == cur) return;
    auto tmp = (*scene_)[cur];
    scene_->erase(scene_->begin() + cur);
    scene_->insert(scene_->begin() + newIndex, tmp);
}

void EditorWindows::drawHierarchyNode(GameObject* node) {
    if (!node) return;

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanFullWidth;

    if (selected_ && selected_.get() == node)
        flags |= ImGuiTreeNodeFlags_Selected;

    bool hasChildren = !node->children.empty();
    if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf;

    bool open = ImGui::TreeNodeEx((void*)node, flags, "%s", node->name.c_str());

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        setSelection(findShared(node));

    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("DND_GAMEOBJECT", &node, sizeof(GameObject*));
        ImGui::Text("Move: %s", node->name.c_str());
        ImGui::EndDragDropSource();
    }
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_GAMEOBJECT")) {
            GameObject* dragged = *(GameObject**)payload->Data;
            if (dragged) reparent(dragged, node);
        }
        ImGui::EndDragDropTarget();
    }

    if (open) {
        for (GameObject* c : node->children) drawHierarchyNode(c);
        ImGui::TreePop();
    }
}

void EditorWindows::drawInspector(Camera* camera, float x, float y, float w, float h) {
    (void)camera;

    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
    if (!ImGui::Begin("Inspector", nullptr, PanelFlags())) { ImGui::End(); return; }

    if (!selected_) { ImGui::TextUnformatted("No selection"); ImGui::End(); return; }

    GameObject* go = selected_.get();
    if (camera && ImGui::Button("Frame Selected"))
    {
        if (go->mesh)
        {
            mat4 W = computeWorldMatrix(go);
            AABB wb = go->mesh->getWorldAABB(W);
            vec3 c = (wb.min + wb.max) * 0.5;
            vec3 e = (wb.max - wb.min) * 0.5;
            double radius = glm::length(e);
            camera->focusOn(c, std::max(0.01, radius));
        }
    }
    ImGui::Separator();

    ImGui::Text("Name:");
    char buf[256];
    std::snprintf(buf, sizeof(buf), "%s", go->name.c_str());
    if (ImGui::InputText("##name", buf, sizeof(buf))) go->name = buf;

    ImGui::Separator();

    vec3 p = go->transform.pos();
    float pf[3] = { (float)p.x, (float)p.y, (float)p.z };
    if (ImGui::DragFloat3("Position", pf, 0.05f))
        go->transform.setPosition(vec3(pf[0], pf[1], pf[2]));

    vec3 s = go->transform.getScale();
    float sf[3] = { (float)s.x, (float)s.y, (float)s.z };
    if (ImGui::DragFloat3("Scale", sf, 0.02f, 0.001f, 1000.0f))
        go->transform.setScale(vec3(sf[0], sf[1], sf[2]));

    if (ImGui::Button("Reset Scale")) go->transform.resetScale();

    static float rotDelta[3] = { 0,0,0 };
    ImGui::DragFloat3("Rotate Delta (deg)", rotDelta, 0.5f);
    if (ImGui::Button("Apply Rotation")) {
        go->transform.rotateEulerDeltaDeg(vec3(rotDelta[0], rotDelta[1], rotDelta[2]));
        rotDelta[0] = rotDelta[1] = rotDelta[2] = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Rotation")) go->transform.resetRotation();

    ImGui::Separator();

    if (go->mesh) {
        ImGui::Text("Mesh: OK");
        ImGui::Text("Vertices: %d", (int)go->mesh->getVertexCount());
        ImGui::Text("Triangles: %d", (int)go->mesh->getTriangleCount());

        unsigned int tex = go->getTextureID();
        ImGui::Text("TextureID: %u", tex);

        if (tex) {
            int tw, th;
            glTextureSize((GLuint)tex, tw, th);
            ImGui::Text("Texture size: %dx%d", tw, th);
        }

        bool showVN = go->mesh->showVertexNormals;
        bool showFN = go->mesh->showFaceNormals;
        if (ImGui::Checkbox("Show Vertex Normals", &showVN)) go->mesh->showVertexNormals = showVN;
        if (ImGui::Checkbox("Show Face Normals", &showFN)) go->mesh->showFaceNormals = showFN;

        float normalLen = (float)go->mesh->normalLength;
        if (ImGui::DragFloat("Normal Length", &normalLen, 0.01f, 0.01f, 10.0f))
            go->mesh->normalLength = normalLen;
    }
    else {
        ImGui::Text("Mesh: (null)");
    }

    ImGui::End();
}
// ---------------- Scene Save/Load + Play/Pause helpers ----------------
std::string EditorWindows::defaultScenePath()
{
    fs::path scenesDir = fs::path(getAssetsPath()) / "Scenes";
    std::error_code ec;
    fs::create_directories(scenesDir, ec);
    return (scenesDir / "scene.m2scene").string();
}

void EditorWindows::doSaveScene(const std::string& path)
{
    if (!scene_) { log("Scene is null"); return; }

    std::string err;
    if (!SceneIO::saveToFile(path, *scene_, &err))
    {
        log(std::string("Save failed: ") + err);
        return;
    }

    currentScenePath_ = path;
    std::snprintf(scenePathBuf_, sizeof(scenePathBuf_), "%s", currentScenePath_.c_str());
    log(std::string("Saved scene: ") + currentScenePath_);
}

void EditorWindows::doLoadScene(const std::string& path)
{
    if (!scene_) { log("Scene is null"); return; }

    std::string err;
    if (!SceneIO::loadFromFile(path, *scene_, getAssetsPath(), &err))
    {
        log(std::string("Load failed: ") + err);
        return;
    }

    currentScenePath_ = path;
    std::snprintf(scenePathBuf_, sizeof(scenePathBuf_), "%s", currentScenePath_.c_str());
    setSelection(nullptr);

    log(std::string("Loaded scene: ") + currentScenePath_);
}

void EditorWindows::drawSceneIOPopups()
{
    // Save As
    if (openSaveScenePopup_)
    {
        openSaveScenePopup_ = false;
        ImGui::OpenPopup("Save Scene As");
    }

    if (ImGui::BeginPopupModal("Save Scene As", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("Path:");
        ImGui::InputText("##save_path", scenePathBuf_, sizeof(scenePathBuf_));

        if (ImGui::Button("Save"))
        {
            doSaveScene(scenePathBuf_);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    // Load
    if (openLoadScenePopup_)
    {
        openLoadScenePopup_ = false;
        ImGui::OpenPopup("Load Scene");
    }

    if (ImGui::BeginPopupModal("Load Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("Path:");
        ImGui::InputText("##load_path", scenePathBuf_, sizeof(scenePathBuf_));

        if (ImGui::Button("Load"))
        {
            doLoadScene(scenePathBuf_);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

void EditorWindows::onPlayPressed()
{
    if (!scene_) return;

    if (playState_ == PlayState::Edit)
    {
        // Snapshot current edit scene (Unity-like: stop restores)
        playBackup_ = SceneIO::serialize(*scene_);
        playState_ = PlayState::Playing;
        log("Play (snapshot created)");
        return;
    }

    // Paused -> resume
    if (playState_ == PlayState::Paused)
    {
        playState_ = PlayState::Playing;
        log("Resume");
    }
}

void EditorWindows::onPausePressed()
{
    if (playState_ == PlayState::Playing)
    {
        playState_ = PlayState::Paused;
        log("Pause");
    }
    else if (playState_ == PlayState::Paused)
    {
        playState_ = PlayState::Playing;
        log("Resume");
    }
}

void EditorWindows::onStopPressed()
{
    if (!scene_) return;

    if (playState_ == PlayState::Edit) return;

    // Restore edit scene snapshot
    std::string err;
    if (!playBackup_.empty() && SceneIO::deserialize(playBackup_, *scene_, getAssetsPath(), &err))
    {
        log("Stop");
    }
    else
    {
        log(std::string("Stop restore failed:") + err);
    }

    playState_ = PlayState::Edit;
    setSelection(nullptr);
    playBackup_.clear();
}

