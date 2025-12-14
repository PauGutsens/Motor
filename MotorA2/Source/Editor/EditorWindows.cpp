// EditorWindows.cpp
// 中英双语注释 / Chinese-English bilingual comments

#include "EditorWindows.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

#include <SDL3/SDL.h>
#include <GL/glew.h> // Must be included before any OpenGL headers / 必须在任何 OpenGL 头文件之前

#include <filesystem>
#include <algorithm>
#include <sstream>

#include "Camera.h"
#include "ModelLoader.h"

namespace fs = std::filesystem;

// 查询纹理尺寸 / helper to query texture size
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

    // 键盘导航 / keyboard navigation
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // ---- 字体：默认 + 中文字体（如果存在） ----
    io.Fonts->AddFontDefault();
    {
        ImFontConfig cfg;
        cfg.MergeMode = true;
        cfg.PixelSnapH = true;
        static const ImWchar ranges[] = {
            0x0020, 0x00FF,   // Basic Latin + Latin-1
            0x4E00, 0x9FA5,   // 常用中日韩统一表意文字
            0
        };

        fs::path fontPath = fs::current_path() / "Assets" / "Fonts" / "NotoSansSC-Regular.otf";
        if (fs::exists(fontPath)) {
            io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), 16.0f, &cfg, ranges);
        }
        // 若不存在该字体，则仍然使用默认字体，中文会显示为 '?'
    }

    ImGui::StyleColorsDark();

    // SDL3 + OpenGL3 backend
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
}

void EditorWindows::processEvent(const SDL_Event& e) {
    ImGui_ImplSDL3_ProcessEvent(&e);
}

void EditorWindows::newFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void EditorWindows::render(Camera* camera) {
    // 顶部菜单 / main menu bar
    drawMainMenuBar();

    if (show_demo_)   ImGui::ShowDemoWindow(&show_demo_);
    if (show_about_)  drawAbout();

    drawHierarchy();
    drawInspector(camera);
    if (show_console_) drawConsole();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorWindows::setScene(std::vector<std::shared_ptr<GameObject>>* scene) {
    scene_ = scene;
}

void EditorWindows::log(const std::string& s) {
    console_.push_back(s);
    LOG_INFO(s); // 你项目里 Logger 的宏
}

// 查找 Assets 路径（向上找 6 层）
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

// 从 Assets/Street 加载 FBX/DAE
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

    std::vector<std::shared_ptr<Mesh>> meshes = ModelLoader::loadModel(p.string());
    if (meshes.empty()) {
        log(std::string("Failed to load model / 加载失败: ") + p.string());
        return;
    }

    std::shared_ptr<GameObject> firstGo = nullptr;
    std::string baseName = p.stem().string();

    for (size_t i = 0; i < meshes.size(); ++i) {
        const std::shared_ptr<Mesh>& mesh = meshes[i];
        if (!mesh) continue;

        std::string goName = (meshes.size() == 1)
            ? baseName
            : (baseName + "_" + std::to_string(i));

        auto go = std::make_shared<GameObject>(goName);
        go->setMesh(mesh);

        go->parent = nullptr;
        go->children.clear();

        scene_->push_back(go);

        if (!firstGo) {
            firstGo = go;
        }
    }

    if (firstGo) {
        setSelection(firstGo);
    }

    log(std::string("Loaded / 已加载: ") + p.string());
}

// 顶部菜单栏
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

// Console 窗口
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

// About 窗口
void EditorWindows::drawAbout() {
    ImGui::SetNextWindowSize(ImVec2(420, 220), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("About", &show_about_)) { ImGui::End(); return; }

    ImGui::Text("MotorA2 - Editor");
    ImGui::Separator();
    ImGui::Text("Controls / 操作:");
    ImGui::BulletText("Right mouse + drag: free look / 右键拖动: 自由旋转视角");
    ImGui::BulletText("Mouse wheel: dolly / 鼠标滚轮: 推拉相机");
    ImGui::BulletText("Middle mouse or Shift+RMB: pan / 中键或 Shift+右键: 平移");
    ImGui::BulletText("Right mouse + WASDQE: fly / 右键 + WASDQE: 飞行相机");
    ImGui::BulletText("Use File menu to load Street assets / 用 File 菜单加载 Street 场景");

    ImGui::End();
}

// 切换选中对象
void EditorWindows::setSelection(const std::shared_ptr<GameObject>& go) {
    if (selected_) selected_->isSelected = false; // 清除旧选中
    selected_ = go;
    if (selected_) selected_->isSelected = true;
}

// 在 scene_ 中根据裸指针找到 shared_ptr
std::shared_ptr<GameObject> EditorWindows::findShared(GameObject* raw) {
    if (!scene_ || !raw) return nullptr;
    for (auto& sp : *scene_) {
        if (sp.get() == raw) return sp;
    }
    return nullptr;
}

// 收集后序遍历（子->父）
void EditorWindows::collectPostorder(GameObject* root, std::vector<GameObject*>& out) {
    if (!root) return;
    for (GameObject* c : root->children) collectPostorder(c, out);
    out.push_back(root);
}

// 从 scene_ 中移除一个 GameObject（根据裸指针）
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

// 删除选中对象及其所有子节点
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

// 改变父节点
void EditorWindows::reparent(GameObject* dragged, GameObject* target) {
    if (!dragged || !target) return;
    if (dragged == target) return;
    if (target->isDescendantOf(dragged)) return; // 防止环

    mat4 M_world = computeWorldMatrix(dragged);

    if (dragged->parent) dragged->parent->removeChild(dragged);
    target->addChild(dragged);

    setLocalFromWorld(dragged, M_world, target);
}

// 在同一父节点下改变顺序
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

// 在根节点层级中改变顺序
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

// 绘制层级窗口
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
        if (sp->parent != nullptr) continue; // 只画根节点
        drawHierarchyNode(sp.get());
    }

    ImGui::End();
}

// 绘制单个节点（递归）
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

    // 点击选择
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        setSelection(findShared(node));
    }

    // 拖拽源
    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("DND_GAMEOBJECT", &node, sizeof(GameObject*));
        ImGui::Text("Move: %s", node->name.c_str());
        ImGui::EndDragDropSource();
    }

    // 拖拽目标
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_GAMEOBJECT")) {
            GameObject* dragged = *(GameObject**)payload->Data;
            if (dragged) {
                reparent(dragged, node); // 改父节点
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

// Inspector 窗口
void EditorWindows::drawInspector(Camera* camera) {
    (void)camera; // 目前没有直接操作相机的控件 / currently unused

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

    // Transform 编辑
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

    static float rotDelta[3] = { 0, 0, 0 };
    ImGui::DragFloat3("Rotate Delta (deg) / 旋转增量(度)", rotDelta, 0.5f);
    if (ImGui::Button("Apply Rotation / 应用旋转")) {
        go->transform.rotateEulerDeltaDeg(
            vec3(rotDelta[0], rotDelta[1], rotDelta[2]));
        rotDelta[0] = rotDelta[1] = rotDelta[2] = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Rotation / 重置旋转")) {
        go->transform.resetRotation();
    }

    ImGui::Separator();

    // Mesh 信息
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

        // Mesh 调试显示：法线
        bool showVN = go->mesh->showVertexNormals;
        bool showFN = go->mesh->showFaceNormals;
        if (ImGui::Checkbox("Show Vertex Normals / 显示顶点法线", &showVN)) {
            go->mesh->showVertexNormals = showVN;
        }
        if (ImGui::Checkbox("Show Face Normals / 显示面法线", &showFN)) {
            go->mesh->showFaceNormals = showFN;
        }
        float normalLen = static_cast<float>(go->mesh->normalLength);
        if (ImGui::DragFloat("Normal Length / 法线长度", &normalLen, 0.01f, 0.01f, 10.0f)) {
            go->mesh->normalLength = normalLen;
        }
    }
    else {
        ImGui::Text("Mesh / 网格: (null)");
    }

    ImGui::End();
}
