// EditorWindows.cpp
// 中英双语注释 / Chinese-English bilingual comments

#include "EditorWindows.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

#include <SDL3/SDL.h>
#include <GL/glew.h>

#include <filesystem>
#include <algorithm>
#include <sstream>

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
    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForOpenGL(window_, gl_);
    ImGui_ImplOpenGL3_Init("#version 330");

    log("Editor initialized / 编辑器初始化完成");
}

void EditorWindows::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    window_ = nullptr;
    gl_ = nullptr;
    scene_ = nullptr;
    selected_.reset();
    selectedRef_ = nullptr;
    mainCam_ = nullptr;
    quit_ = false;
}

void EditorWindows::processEvent(const SDL_Event& e) {
    ImGui_ImplSDL3_ProcessEvent(&e);
    if (e.type == SDL_EVENT_QUIT) {
        quit_ = true;
    }
}

void EditorWindows::newFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void EditorWindows::render(Camera* camera) {
    drawMainMenuBar();

    if (show_demo_) ImGui::ShowDemoWindow(&show_demo_);
    if (show_about_) drawAbout();

    drawHierarchy();
    drawInspector(camera);
    if (show_console_) drawConsole();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorWindows::setScene(std::vector<std::shared_ptr<GameObject>>* scene) {
    scene_ = scene;
}

void EditorWindows::setScene(std::vector<std::shared_ptr<GameObject>>* scene, std::shared_ptr<GameObject>& selected) {
    scene_ = scene;
    selectedRef_ = &selected;

    selected_ = selected;
    if (selected_) selected_->isSelected = true;
}

void EditorWindows::setMainCamera(Camera* cam) {
    mainCam_ = cam;
}

bool EditorWindows::wantsQuit() const {
    return quit_;
}

void EditorWindows::log(const std::string& s) {
    console_.push_back(s);
    // ✅ 修复：Logger 没有静态 Log()，用 instance().log()
    Logger::instance().log(LogLevel::Info, s);
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
    if (!scene_) {
        log("Scene is null / scene_ 为空，先 setScene()");
        return;
    }

    fs::path p = fs::path(getAssetsPath()) / "Street" / filename;
    if (!fs::exists(p)) {
        log(std::string("Asset not found / 找不到资源: ") + p.string());
        return;
    }

    // ✅ 修复：ModelLoader 只有 loadModel()，并且返回 vector<shared_ptr<Mesh>>
    auto meshes = ModelLoader::loadModel(p.string());
    std::shared_ptr<Mesh> mesh = meshes.empty() ? nullptr : meshes.front();
    if (!mesh) {
        log(std::string("Failed to load model / 加载失败: ") + p.string());
        return;
    }

    auto go = std::make_shared<GameObject>(p.stem().string());
    go->setMesh(mesh);

    go->parent = nullptr;
    go->children.clear();

    scene_->push_back(go);
    setSelection(go);

    log(std::string("Loaded / 已加载: ") + p.string());
}

void EditorWindows::drawMainMenuBar() {
    if (!ImGui::BeginMainMenuBar()) return;

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Load Street environment (FBX)", nullptr)) {
            loadStreetAsset("Street environment_V01.FBX");
        }
        if (ImGui::MenuItem("Load street2 (FBX)", nullptr)) {
            loadStreetAsset("street2.FBX");
        }
        if (ImGui::MenuItem("Load scene (DAE)", nullptr)) {
            loadStreetAsset("scene.DAE");
        }
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

void EditorWindows::drawConsole() {
    ImGui::SetNextWindowSize(ImVec2(600, 220), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Console", &show_console_)) { ImGui::End(); return; }

    static bool auto_scroll = true;
    ImGui::Checkbox("Auto-scroll / 自动滚动", &auto_scroll);
    ImGui::Separator();

    ImGui::BeginChild("console_scroller", ImVec2(0, 0), false);
    for (const auto& line : console_) {
        ImGui::TextUnformatted(line.c_str());
    }
    if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();

    ImGui::End();
}

void EditorWindows::drawAbout() {
    ImGui::SetNextWindowSize(ImVec2(420, 160), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("About", &show_about_)) { ImGui::End(); return; }
    ImGui::Text("MotorA2 - Editor");
    ImGui::Separator();
    ImGui::Text("Controls / 操作:");
    ImGui::BulletText("Left click select / 左键选择");
    ImGui::BulletText("Hierarchy drag-drop / 层级拖拽（换父节点/排序）");
    ImGui::BulletText("Use File menu to load Street / 用 File 菜单加载 Street");
    ImGui::End();
}

void EditorWindows::setSelection(const std::shared_ptr<GameObject>& go) {
    if (selected_) selected_->isSelected = false;

    selected_ = go;
    if (selectedRef_) *selectedRef_ = selected_;

    if (selected_) selected_->isSelected = true;
}

std::shared_ptr<GameObject> EditorWindows::findShared(GameObject* raw) {
    if (!scene_ || !raw) return nullptr;
    for (auto& sp : *scene_) {
        if (sp.get() == raw) return sp;
    }
    return nullptr;
}

void EditorWindows::collectPostorder(GameObject* root, std::vector<GameObject*>& out) {
    if (!root) return;
    for (GameObject* c : root->children) collectPostorder(c, out);
    out.push_back(root);
}

void EditorWindows::removeFromScene(GameObject* raw) {
    if (!scene_ || !raw) return;

    if (raw->parent) {
        raw->parent->removeChild(raw);
    }

    scene_->erase(
        std::remove_if(scene_->begin(), scene_->end(),
            [raw](const std::shared_ptr<GameObject>& sp) {
                return sp.get() == raw;
            }),
        scene_->end()
    );
}

void EditorWindows::deleteSelectedRecursive() {
    if (!scene_ || !selected_) return;

    std::vector<GameObject*> post;
    collectPostorder(selected_.get(), post);

    setSelection(nullptr);

    for (GameObject* n : post) {
        removeFromScene(n);
    }

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
    for (int i = 0; i < (int)v.size(); ++i) {
        if (v[i] == node) { cur = i; break; }
    }
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
    for (int i = 0; i < (int)scene_->size(); ++i) {
        if ((*scene_)[i].get() == node) { cur = i; break; }
    }
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

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        setSelection(findShared(node));
    }

    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("DND_GAMEOBJECT", &node, sizeof(GameObject*));
        ImGui::Text("Move: %s", node->name.c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_GAMEOBJECT")) {
            GameObject* dragged = *(GameObject**)payload->Data;
            if (dragged) {
                reparent(dragged, node);
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (open) {
        for (GameObject* c : node->children) {
            drawHierarchyNode(c);
        }
        ImGui::TreePop();
    }
}

void EditorWindows::drawHierarchy() {
    ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Hierarchy")) { ImGui::End(); return; }

    if (!scene_) {
        ImGui::TextUnformatted("Scene is null / scene_ 为空");
        ImGui::End();
        return;
    }

    if (ImGui::Button("Delete Selected / 删除选中")) {
        deleteSelectedRecursive();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Street / 加载 Street")) {
        loadStreetAsset("Street environment_V01.FBX");
    }

    ImGui::Separator();

    for (auto& sp : *scene_) {
        if (!sp) continue;
        if (sp->parent != nullptr) continue;
        drawHierarchyNode(sp.get());
    }

    ImGui::End();
}

void EditorWindows::drawInspector(Camera* camera) {
    ImGui::SetNextWindowSize(ImVec2(380, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Inspector")) { ImGui::End(); return; }

    if (!selected_) {
        ImGui::TextUnformatted("No selection / 未选中对象");
        ImGui::End();
        return;
    }

    GameObject* go = selected_.get();
    ImGui::Text("Name / 名称:");
    char buf[256];
    std::snprintf(buf, sizeof(buf), "%s", go->name.c_str());
    if (ImGui::InputText("##name", buf, sizeof(buf))) {
        go->name = buf;
    }

    ImGui::Separator();

    vec3 p = go->transform.pos();
    float pf[3] = { (float)p.x, (float)p.y, (float)p.z };
    if (ImGui::DragFloat3("Position / 位置", pf, 0.05f)) {
        go->transform.setPosition(vec3(pf[0], pf[1], pf[2]));
    }

    vec3 s = go->transform.getScale();
    float sf[3] = { (float)s.x, (float)s.y, (float)s.z };
    if (ImGui::DragFloat3("Scale / 缩放", sf, 0.02f, 0.001f, 1000.0f)) {
        go->transform.setScale(vec3(sf[0], sf[1], sf[2]));
    }
    if (ImGui::Button("Reset Scale / 重置缩放")) {
        go->transform.resetScale();
    }

    static float rotDelta[3] = { 0,0,0 };
    ImGui::DragFloat3("Rotate Delta (deg) / 旋转增量(度)", rotDelta, 0.5f);
    if (ImGui::Button("Apply Rotation / 应用旋转")) {
        go->transform.rotateEulerDeltaDeg(vec3(rotDelta[0], rotDelta[1], rotDelta[2]));
        rotDelta[0] = rotDelta[1] = rotDelta[2] = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Rotation / 重置旋转")) {
        go->transform.resetRotation();
    }

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
    }
    else {
        ImGui::Text("Mesh / 网格: (null)");
    }

    (void)camera; // 目前 Inspector 不强依赖 camera
    ImGui::End();
}
