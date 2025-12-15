#include "EditorWindows.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

#include <SDL3/SDL.h>
#include <GL/glew.h>

#include <filesystem>
#include <algorithm>

#include "Camera.h"
#include "ModelLoader.h"

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

    log("Editor initialized / 编辑器初始化完成");
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

void EditorWindows::loadStreetAsset(const std::string& filename) {
    if (!scene_) { log("Scene is null / scene_ 为空，先 setScene()"); return; }

    fs::path p = fs::path(getAssetsPath()) / "Street" / filename;
    if (!fs::exists(p)) { log(std::string("Asset not found / 找不到资源: ") + p.string()); return; }

    auto meshes = ModelLoader::loadModel(p.string());
    if (meshes.empty()) { log(std::string("Failed to load model / 加载失败: ") + p.string()); return; }

    std::shared_ptr<GameObject> firstGo = nullptr;
    std::string baseName = p.stem().string();

    for (size_t i = 0; i < meshes.size(); ++i) {
        auto& mesh = meshes[i];
        if (!mesh) continue;

        std::string goName = (meshes.size() == 1) ? baseName : (baseName + "_" + std::to_string(i));
        auto go = std::make_shared<GameObject>(goName);
        go->setMesh(mesh);

        go->parent = nullptr;
        go->children.clear();

        scene_->push_back(go);
        if (!firstGo) firstGo = go;
    }

    if (firstGo) setSelection(firstGo);
    log(std::string("Loaded / 已加载: ") + p.string());
}

void EditorWindows::drawMainMenuBar() {
    if (!ImGui::BeginMainMenuBar()) return;

    if (ImGui::BeginMenu("File")) {
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
        log("Viewport FBO incomplete / Viewport 帧缓冲不完整");
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
    drawHierarchy(wp.x, wp.y, leftW, centerH);
    drawInspector(camera, wp.x + ws.x - rightW, wp.y, rightW, centerH);
    if (show_console_) drawConsole(wp.x, wp.y + centerH, ws.x, bottomH);

    // Viewport uses the center area
    drawViewportWindow(centerX, centerY, centerW, centerH);

    if (show_demo_) ImGui::ShowDemoWindow(&show_demo_);
    if (show_about_) drawAbout();
}

static ImGuiWindowFlags PanelFlags() {
    return ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse;
}

void EditorWindows::drawViewportWindow(float x, float y, float w, float h) {
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

    ImGui::End();
}

void EditorWindows::drawHierarchy(float x, float y, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
    if (!ImGui::Begin("Hierarchy", nullptr, PanelFlags())) { ImGui::End(); return; }

    if (!scene_) { ImGui::TextUnformatted("Scene is null / scene_ 为空"); ImGui::End(); return; }

    if (ImGui::Button("Delete Selected / 删除选中")) deleteSelectedRecursive();
    ImGui::SameLine();
    if (ImGui::Button("Load Street / 加载 Street")) loadStreetAsset("Street environment_V01.FBX");
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
    ImGui::Checkbox("Auto-scroll / 自动滚动", &auto_scroll);
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
    log("Deleted selection subtree / 已删除选中子树");
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

    if (!selected_) { ImGui::TextUnformatted("No selection / 未选中对象"); ImGui::End(); return; }

    GameObject* go = selected_.get();

    ImGui::Text("Name / 名称:");
    char buf[256];
    std::snprintf(buf, sizeof(buf), "%s", go->name.c_str());
    if (ImGui::InputText("##name", buf, sizeof(buf))) go->name = buf;

    ImGui::Separator();

    vec3 p = go->transform.pos();
    float pf[3] = { (float)p.x, (float)p.y, (float)p.z };
    if (ImGui::DragFloat3("Position / 位置", pf, 0.05f))
        go->transform.setPosition(vec3(pf[0], pf[1], pf[2]));

    vec3 s = go->transform.getScale();
    float sf[3] = { (float)s.x, (float)s.y, (float)s.z };
    if (ImGui::DragFloat3("Scale / 缩放", sf, 0.02f, 0.001f, 1000.0f))
        go->transform.setScale(vec3(sf[0], sf[1], sf[2]));

    if (ImGui::Button("Reset Scale / 重置缩放")) go->transform.resetScale();

    static float rotDelta[3] = { 0,0,0 };
    ImGui::DragFloat3("Rotate Delta (deg) / 旋转增量(度)", rotDelta, 0.5f);
    if (ImGui::Button("Apply Rotation / 应用旋转")) {
        go->transform.rotateEulerDeltaDeg(vec3(rotDelta[0], rotDelta[1], rotDelta[2]));
        rotDelta[0] = rotDelta[1] = rotDelta[2] = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Rotation / 重置旋转")) go->transform.resetRotation();

    ImGui::Separator();

    if (go->mesh) {
        ImGui::Text("Mesh / 网格: OK");
        ImGui::Text("Vertices / 顶点: %d", (int)go->mesh->getVertexCount());
        ImGui::Text("Triangles / 三角形: %d", (int)go->mesh->getTriangleCount());

        unsigned int tex = go->getTextureID();
        ImGui::Text("TextureID / 纹理ID: %u", tex);

        if (tex) {
            int tw, th;
            glTextureSize((GLuint)tex, tw, th);
            ImGui::Text("Texture size / 纹理尺寸: %dx%d", tw, th);
        }

        bool showVN = go->mesh->showVertexNormals;
        bool showFN = go->mesh->showFaceNormals;
        if (ImGui::Checkbox("Show Vertex Normals / 显示顶点法线", &showVN)) go->mesh->showVertexNormals = showVN;
        if (ImGui::Checkbox("Show Face Normals / 显示面法线", &showFN)) go->mesh->showFaceNormals = showFN;

        float normalLen = (float)go->mesh->normalLength;
        if (ImGui::DragFloat("Normal Length / 法线长度", &normalLen, 0.01f, 0.01f, 10.0f))
            go->mesh->normalLength = normalLen;
    }
    else {
        ImGui::Text("Mesh / 网格: (null)");
    }

    ImGui::End();
}
