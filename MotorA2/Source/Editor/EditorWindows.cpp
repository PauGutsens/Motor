// EditorWindows.cpp
// 中英双语注释 / Chinese-English bilingual comments

#include "EditorWindows.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

#include <SDL3/SDL.h>
//#include <SDL3/SDL_opengl.h>
#include <GL/glew.h> // Must be included before any OpenGL headers / 必须在任何 OpenGL 头文件之前

#include <filesystem>
#include <algorithm>
#include <sstream>

#include "Camera.h"
#include "ModelLoader.h"

namespace fs = std::filesystem;

static void glTextureSize(GLuint tex, int& w, int& h) {
    // Query texture size / 查询纹理尺寸
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

    // 你可以按需要打开键盘导航等
    // Enable keyboard navigation if you want
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    // SDL3 + OpenGL3 backend
    ImGui_ImplSDL3_InitForOpenGL(window_, gl_);
    ImGui_ImplOpenGL3_Init("#version 330");

    log("Editor initialized / 编辑器初始化完成");
}

void EditorWindows::shutdown() {
    // Destroy ImGui backends / 释放 ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    window_ = nullptr;
    gl_ = nullptr;
    scene_ = nullptr;
    selected_.reset();
}

void EditorWindows::processEvent(const SDL_Event& e) {
    // Feed SDL events to ImGui / 把 SDL 事件交给 ImGui
    ImGui_ImplSDL3_ProcessEvent(&e);
}

void EditorWindows::newFrame() {
    // Start new ImGui frame / 开始新一帧
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void EditorWindows::render(Camera* camera) {
    // Main UI / 主界面
    drawMainMenuBar();

    if (show_demo_) ImGui::ShowDemoWindow(&show_demo_);
    if (show_about_) drawAbout();

    drawHierarchy();
    drawInspector(camera);
    if (show_console_) drawConsole();

    // Render ImGui / 渲染 ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorWindows::setScene(std::vector<std::shared_ptr<GameObject>>* scene) {
    scene_ = scene;
}

void EditorWindows::log(const std::string& s) {
    console_.push_back(s);
    Logger::Log(s.c_str());
}

std::string EditorWindows::getAssetsPath() {
    // Default: <project>/Assets
    // 默认：工程根目录下的 Assets
    // 这里用当前工作目录推断：通常 VS/CMake 调试时工作目录是 build/.../Debug
    // We infer by current_path()
    fs::path cwd = fs::current_path();
    // Try to find "Assets" in parents / 向上找 Assets
    for (int i = 0; i < 6; ++i) {
        fs::path candidate = cwd / "Assets";
        if (fs::exists(candidate) && fs::is_directory(candidate)) {
            return candidate.string();
        }
        cwd = cwd.parent_path();
        if (cwd.empty()) break;
    }
    // Fallback / 兜底：直接用 ./Assets
    return (fs::current_path() / "Assets").string();
}

void EditorWindows::loadStreetAsset(const std::string& filename) {
    if (!scene_) {
        log("Scene is null / scene_ 为空，先 setScene()");
        return;
    }

    // Assets/Street/<filename>
    fs::path p = fs::path(getAssetsPath()) / "Street" / filename;
    if (!fs::exists(p)) {
        log(std::string("Asset not found / 找不到资源: ") + p.string());
        return;
    }

    // Use your ModelLoader to load mesh
    // 使用你的 ModelLoader 加载 Mesh
    std::shared_ptr<Mesh> mesh = ModelLoader::LoadModel(p.string());
    if (!mesh) {
        log(std::string("Failed to load model / 加载失败: ") + p.string());
        return;
    }

    auto go = std::make_shared<GameObject>(p.stem().string());
    go->setMesh(mesh);

    // Root object: parent=null, children empty
    // 根对象：parent=null，children 空
    go->parent = nullptr;
    go->children.clear();

    // Put into scene ownership list / 放进 scene_ 的所有权列表
    scene_->push_back(go);
    setSelection(go);

    log(std::string("Loaded / 已加载: ") + p.string());
}

void EditorWindows::drawMainMenuBar() {
    if (!ImGui::BeginMainMenuBar()) return;

    if (ImGui::BeginMenu("File")) {
        // 你老师让用 Street：给两个常用入口
        // Street pack shortcuts
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
    // Clear previous selection flag / 清除旧选择
    if (selected_) selected_->isSelected = false;

    selected_ = go;

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
    // children is a vector<GameObject*> in your GameObject
    // children 是 vector<GameObject*>（你的实现）
    for (GameObject* c : root->children) collectPostorder(c, out);
    out.push_back(root);
}

void EditorWindows::removeFromScene(GameObject* raw) {
    if (!scene_ || !raw) return;

    // Detach from parent / 从父节点断开
    if (raw->parent) {
        raw->parent->removeChild(raw);
    }

    // Remove from ownership list / 从 scene_ 所有权列表移除 shared_ptr
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

    // Postorder: children first / 后序：先子后父
    std::vector<GameObject*> post;
    collectPostorder(selected_.get(), post);

    // Clear selection early / 先清空选择，避免悬空
    setSelection(nullptr);

    // Remove every node / 逐个移除
    for (GameObject* n : post) {
        removeFromScene(n);
    }

    log("Deleted selection subtree / 已删除选中子树");
}

void EditorWindows::reparent(GameObject* dragged, GameObject* target) {
    if (!dragged || !target) return;
    if (dragged == target) return;

    // Prevent cycles / 防止环
    if (target->isDescendantOf(dragged)) return;

    // Preserve world matrix / 保持世界矩阵
    mat4 M_world = computeWorldMatrix(dragged);

    // Detach from old parent if any / 从旧父断开
    if (dragged->parent) dragged->parent->removeChild(dragged);

    // Attach to new parent / 挂到新父
    target->addChild(dragged);

    // Recompute local from world (new parent) / 用世界矩阵反推本地矩阵
    setLocalFromWorld(dragged, M_world, target);
}

void EditorWindows::reorderSibling(GameObject* node, GameObject* parent, int newIndex) {
    if (!node || !parent) return;
    auto& v = parent->children;

    // Find current index / 找到当前下标
    int cur = -1;
    for (int i = 0; i < (int)v.size(); ++i) {
        if (v[i] == node) { cur = i; break; }
    }
    if (cur < 0) return;

    // Clamp newIndex / 夹紧范围
    newIndex = std::max(0, std::min(newIndex, (int)v.size() - 1));
    if (newIndex == cur) return;

    // Move element / 移动元素
    GameObject* tmp = v[cur];
    v.erase(v.begin() + cur);
    v.insert(v.begin() + newIndex, tmp);
}

void EditorWindows::reorderRoot(GameObject* node, int newIndex) {
    if (!scene_ || !node) return;

    // Find shared_ptr / 找 shared_ptr
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

    // If selected / 如果选中
    if (selected_ && selected_.get() == node)
        flags |= ImGuiTreeNodeFlags_Selected;

    bool hasChildren = !node->children.empty();
    if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf;

    bool open = ImGui::TreeNodeEx((void*)node, flags, "%s", node->name.c_str());

    // Click to select / 点击选择
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        setSelection(findShared(node));
    }

    // Drag source / 拖拽源
    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("DND_GAMEOBJECT", &node, sizeof(GameObject*));
        ImGui::Text("Move: %s", node->name.c_str());
        ImGui::EndDragDropSource();
    }

    // Drop target / 拖拽目标
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_GAMEOBJECT")) {
            GameObject* dragged = *(GameObject**)payload->Data;
            if (dragged) {
                // If dropped on node: reparent under node
                // 拖到某节点上：作为它的子节点
                reparent(dragged, node);
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Children / 子节点
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

    // Buttons / 按钮
    if (ImGui::Button("Delete Selected / 删除选中")) {
        deleteSelectedRecursive();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Street / 加载 Street")) {
        loadStreetAsset("Street environment_V01.FBX");
    }

    ImGui::Separator();

    // Draw root objects / 绘制根对象
    for (auto& sp : *scene_) {
        // root objects only (parent == nullptr)
        // 只画根对象（parent == nullptr）
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

    // Transform editing / Transform 编辑
    // 你的 Transform 不是 position/rotation/scale 三字段，而是 mat4 + pos()/setScale()/rotateEulerDeltaDeg()
    // Your Transform stores mat4 and helper APIs

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

    // Rotation (apply delta euler) / 旋转（增量欧拉角）
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

    // Mesh info / Mesh 信息
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

    ImGui::End();
}
