#include "EditorWindows.h"
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include "ModelLoader.h"
#include "Camera.h"
#include "AssetDatabase.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using std::string;
namespace fs = std::filesystem;

static void glTextureSize(GLuint tex, int& w, int& h) {
    w = h = 0;
    if (!tex) return;
    glBindTexture(GL_TEXTURE_2D, tex);
    GLint tw = 0, th = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tw);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &th);
    w = tw; h = th;
}

void EditorWindows::init(SDL_Window* window, SDL_GLContext gl) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForOpenGL(window, gl);
    ImGui_ImplOpenGL3_Init("#version 130");
    
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Optional: Multi-viewport
    
    LOG_INFO("ImGui initialized (OpenGL3 backend).");
}

void EditorWindows::shutdown() {
    if (checker_tex_) { glDeleteTextures(1, &checker_tex_); checker_tex_ = 0; }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}


void EditorWindows::setScene(std::vector<std::shared_ptr<GameObject>>* scene,
    std::shared_ptr<GameObject>* selected) {
    scene_ = scene;
    selected_ = selected;
}



void EditorWindows::render(bool* isPlaying, bool* isPaused, bool* step) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGuiIO& io = ImGui::GetIO();
    fps_history_[fps_index_] = io.Framerate;
    fps_index_ = (fps_index_ + 1) % kFpsHistory;
    
    // [NEW] DockSpace
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
    
    drawMainMenu();
    
    // [NEW] Toolbar inside frame scope
    if (isPlaying && isPaused && step) {
        drawToolbar(*isPlaying, *isPaused, *step);
    }
    
    // [NEW] Draw Game View if texture available
    if (gameTexID_ != 0) {
        drawGameWindow(gameTexID_, gameW_, gameH_, isPlaying ? *isPlaying : false);
    }
    
    // [NEW] Draw Scene View
    if (sceneTexID_ != 0) {
        drawSceneWindow(sceneTexID_, sceneW_, sceneH_);
    }

    if (show_console_)   drawConsole();
    if (show_config_)    drawConfig();
    if (show_hierarchy_) drawHierarchy();
    if (show_inspector_) drawInspector();
    if (show_assets_)    drawAssets();
    if (show_about_) {
        if (ImGui::Begin("About", &show_about_)) {
            ImGui::TextUnformatted("Motor");
            ImGui::TextUnformatted("Version: 0.1.0");
            ImGui::Separator();
            ImGui::TextUnformatted("Team Members: Pau Gutsens, Pau Hernandez, Ao Yunqian, Eduard Garcia");
            ImGui::TextUnformatted("Libraries: SDL3, OpenGL, GLEW, ImGui, Assimp, DevIL");
            ImGui::TextUnformatted("License: MIT");
            ImGui::End();
        }
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorWindows::drawMainMenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit", "Esc")) wants_quit_ = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Console", nullptr, &show_console_);
            ImGui::MenuItem("Config", nullptr, &show_config_);
            ImGui::MenuItem("Hierarchy", nullptr, &show_hierarchy_);
            ImGui::MenuItem("Inspector", nullptr, &show_inspector_);
            ImGui::MenuItem("Assets", nullptr, &show_assets_);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Primitives")) {
            if (ImGui::MenuItem("Load Cube"))      loadPrimitiveFromAssets("Cube.fbx");
            if (ImGui::MenuItem("Load Sphere"))    loadPrimitiveFromAssets("Sphere.fbx");
            if (ImGui::MenuItem("Load Cone"))      loadPrimitiveFromAssets("Cone.fbx");
            if (ImGui::MenuItem("Load Cylinder"))  loadPrimitiveFromAssets("Cylinder.fbx");
            if (ImGui::MenuItem("Load Torus"))     loadPrimitiveFromAssets("Torus.fbx");
            if (ImGui::MenuItem("Load Plane"))     loadPrimitiveFromAssets("Plane.fbx");
            if (ImGui::MenuItem("Load Disc"))      loadPrimitiveFromAssets("Disc.fbx");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            static const char* REPO = "https://github.com/PauGutsens/Motor";
            if (ImGui::MenuItem("GitHub Docs"))       SDL_OpenURL((std::string(REPO)).c_str());
            if (ImGui::MenuItem("Report a bug"))      SDL_OpenURL((std::string(REPO) + "/issues").c_str());
            if (ImGui::MenuItem("Download latest"))   SDL_OpenURL((std::string(REPO) + "/releases").c_str());
            ImGui::Separator();
            ImGui::MenuItem("About", nullptr, &show_about_);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

// Helper to center widgets
static void CenterCursor(float width) {
    float off = (ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x - width) * 0.5f;
    if (off > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
}

void EditorWindows::drawToolbar(bool& isPlaying, bool& isPaused, bool& step) {
    ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    
    float buttonW = 40.0f;
    float totalW = buttonW * 3 + 20; // Approx width
    CenterCursor(totalW);

    // Play/Stop Button
    if (!isPlaying) {
        if (ImGui::Button(" |> ", ImVec2(buttonW, 0))) {
            isPlaying = true;
            isPaused = false;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Play");
    } else {
        // Active Play State
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 1.0f, 1.0f)); // Blue-ish
        if (ImGui::Button(" [] ", ImVec2(buttonW, 0))) {
            isPlaying = false;
            isPaused = false;
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop");
    }

    ImGui::SameLine();

    // Pause/Resume Button
    if (isPlaying && isPaused) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.8f, 0.2f, 1.0f)); // Orange
        if (ImGui::Button(" |> ", ImVec2(buttonW, 0))) {
             isPaused = false;
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Resume");
    } else {
        ImGui::BeginDisabled(!isPlaying);
        if (ImGui::Button(" || ", ImVec2(buttonW, 0))) {
             isPaused = true;
        }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Pause");
    }

    ImGui::SameLine();

    // Step Button
    ImGui::BeginDisabled(!isPlaying || !isPaused);
    if (ImGui::Button(" >| ", ImVec2(buttonW, 0))) {
        step = true;
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Step Frame");

    // Visual Indicator for Play Mode
    if (isPlaying) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "  (GAME MODE)");
        
        // Use a window background tint? 
        // ImGui::GetBackgroundDrawList()->AddRectFilled(...) // Maybe too complex for now
    }

    ImGui::End();
}

void EditorWindows::drawConsole() {
    ImGui::SetNextWindowSize(ImVec2(600, 220), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Console", &show_console_)) { ImGui::End(); return; }
    static ImGuiTextFilter filter;
    filter.Draw("Filter");
    ImGui::Separator();
    assetsFilter_.Draw("Filter##assets");
    ImGui::Separator();
    std::string joined;
    {
        auto logs = Logger::instance().snapshot();
        joined.reserve(1024);
        for (auto& e : logs) {
            if (!filter.PassFilter(e.text.c_str())) continue;
            joined += e.text;
            joined += "\n";
        }
    }
    static std::vector<char> consoleBuf;
    consoleBuf.assign(joined.begin(), joined.end());
    consoleBuf.push_back('\0');
    ImGui::InputTextMultiline("##console_copy", consoleBuf.data(), (size_t)consoleBuf.size(), ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly);
    ImGui::Separator();
    if (ImGui::Button("Clear")) {
        Logger::instance().setMaxEntries(0);
        Logger::instance().setMaxEntries(1000);
    }
    ImGui::End();
}

void EditorWindows::drawConfig() {
    ImGui::SetNextWindowSize(ImVec2(420, 500), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Config", &show_config_)) { ImGui::End(); return; }
    
    // [NEW] Camera Controls
    if (ImGui::CollapsingHeader("Camera Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Editor Camera Controls");
        ImGui::SliderFloat("Move Speed", &cameraMoveSpeed, 1.0f, 500.0f);
        ImGui::SliderFloat("Look Sensitivity", &cameraSensitivity, 0.01f, 5.0f);
        ImGui::SliderFloat("Zoom Speed", &cameraZoomSpeed, 0.1f, 50.0f);
        
        if (ImGui::Button("Reset Camera Defaults")) {
            cameraMoveSpeed = 20.0f;
            cameraSensitivity = 0.1f;
            cameraZoomSpeed = 2.0f;
        }
    }
    ImGui::Separator();

    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::PlotLines("Framerate", fps_history_, kFpsHistory, fps_index_, nullptr, 0.0f, 200.0f, ImVec2(-1, 80));
    ImGui::Separator();
    ImGui::Text("OpenGL : %s", (const char*)glGetString(GL_VERSION));
    ImGui::Text("GPU    : %s", (const char*)glGetString(GL_RENDERER));
    ImGui::Text("Vendor : %s", (const char*)glGetString(GL_VENDOR));
    ImGui::Separator();
    ImGui::Text("SDL CPU cores : %d", SDL_GetNumLogicalCPUCores());
    ImGui::Text("SDL SystemRAM : %d MB", SDL_GetSystemRAM());
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Debug Visualization");
    ImGui::Separator();
    if (ImGui::Checkbox("Show AABBs", &show_aabbs_)) {
        LOG_INFO(show_aabbs_ ? "AABB visualization enabled" : "AABB visualization disabled");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Show Axis-Aligned Bounding Boxes around objects");
    }
    if (ImGui::Checkbox("Enable Frustum Culling", &enable_frustum_culling_)) {
        LOG_INFO(enable_frustum_culling_ ? "Frustum culling enabled" : "Frustum culling disabled");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Skip rendering objects outside camera view");
    }
    if (ImGui::Checkbox("Show Frustum", &show_frustum_)) {
        LOG_INFO(show_frustum_ ? "Frustum visualization enabled" : "Frustum visualization disabled");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Show camera frustum wireframe");
    }
    ImGui::End();
}
void EditorWindows::drawHierarchy() {
    ImGui::SetNextWindowSize(ImVec2(260, 420), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Hierarchy", &show_hierarchy_)) { ImGui::End(); return; }
    if (!scene_) { ImGui::TextDisabled("No scene."); ImGui::End(); return; }

    if (ImGui::Button("Create Empty")) {
        auto go = std::make_shared<GameObject>("Empty");
        scene_->push_back(go);
        if (selected_ && *selected_) {
            GameObject* parent = selected_->get();
            parent->addChild(go.get());
            openNodes_.insert(parent);
        }
        setSelection(go);
        pendingFocus_ = go.get();
    }
    ImGui::SameLine();

    bool hasSel = selected_ && (*selected_);
    ImGui::BeginDisabled(!hasSel);
    if (ImGui::Button("Delete")) {
        if (hasSel) {
            pendingDelete_ = selected_->get();
            if (!pendingDelete_->children.empty())
                ImGui::OpenPopup("Confirm Delete");
            else
                deleteSelectedRecursive();
        }
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::Checkbox("Preserve World", &preserve_world_);

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
            if (hasSel) {
                pendingDelete_ = selected_->get();
                if (!pendingDelete_->children.empty())
                    ImGui::OpenPopup("Confirm Delete");
                else
                    deleteSelectedRecursive();
            }
        }
    }

    if (ImGui::BeginPopupModal("Confirm Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        int total = 1;
        if (pendingDelete_) {
            std::vector<GameObject*> tmp;
            collectPostorder(pendingDelete_, tmp);
            total = (int)tmp.size();
        }
        ImGui::Text("Eliminar '%s' y %d hijos?",
            pendingDelete_ ? pendingDelete_->name.c_str() : "<none>", total - 1);
        if (ImGui::Button("Eliminar", ImVec2(120, 0))) { deleteSelectedRecursive(); ImGui::CloseCurrentPopup(); }
        ImGui::SameLine();
        if (ImGui::Button("Cancelar", ImVec2(120, 0))) { pendingDelete_ = nullptr; ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }

    ImGui::Separator();

    std::vector<GameObject*> roots;
    roots.reserve(scene_->size());
    for (auto& sp : *scene_) {
        GameObject* go = sp.get();
        if (go->parent == nullptr) roots.push_back(go);
    }

    ImGui::BeginChild("HierarchyTree", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (auto* r : roots) drawHierarchyNode(r);

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GO_PTR")) {
            GameObject* dragged = *(GameObject* const*)payload->Data;
            if (dragged && dragged->parent) {
                reparent(dragged, nullptr);
                auto sp = findShared(dragged);
                if (sp) setSelection(sp);
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::EndChild();
    ImGui::End();
}

void EditorWindows::drawHierarchyNode(GameObject* go) {
    using namespace ImGui;

    PushID((void*)((uintptr_t)go ^ 0xABCD));
    ImVec2 avail = GetContentRegionAvail();
    InvisibleButton("##drop-above", ImVec2(avail.x, 3.0f));
    if (BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = AcceptDragDropPayload("GO_PTR")) {
            GameObject* dragged = *(GameObject* const*)payload->Data;
            if (dragged && dragged != go) {
                if (go->parent && dragged->parent == go->parent) {
                    int idx = go->indexInParent();
                    reorderSibling(dragged, go->parent, idx);
                }
                else if (!go->parent && !dragged->parent) {
                    int newIndex = 0;
                    if (scene_) {
                        std::vector<GameObject*> roots;
                        for (auto& sp : *scene_) if (sp->parent == nullptr) roots.push_back(sp.get());
                        for (int i = 0; i < (int)roots.size(); ++i) if (roots[i] == go) { newIndex = i; break; }
                    }
                    reorderRoot(dragged, newIndex);
                }
                auto sp = findShared(dragged);
                if (sp) setSelection(sp);
            }
        }
        EndDragDropTarget();
    }
    PopID();

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
        | ImGuiTreeNodeFlags_OpenOnDoubleClick
        | ImGuiTreeNodeFlags_SpanFullWidth;
    if (go->children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;
    bool isOpen = openNodes_.count(go) > 0;
    SetNextItemOpen(isOpen, ImGuiCond_Always);
    bool open = TreeNodeEx((void*)go, flags, "%s", go->name.c_str());

    if (BeginPopupContextItem()) {
        if (MenuItem("Create Empty")) {
            auto child = std::make_shared<GameObject>("Empty");
            if (scene_) scene_->push_back(child);
            go->addChild(child.get());
            openNodes_.insert(go);
            setSelection(child);
            pendingFocus_ = child.get();
        }
        EndPopup();
    }

    if (IsItemClicked()) {
        if (scene_) {
            auto sp = findShared(go);
            if (sp) setSelection(sp);
        }
    }

    if (BeginDragDropSource()) {
        SetDragDropPayload("GO_PTR", &go, sizeof(GameObject*));
        TextUnformatted(go->name.c_str());
        EndDragDropSource();
    }

    if (BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = AcceptDragDropPayload("GO_PTR")) {
            GameObject* dragged = *(GameObject* const*)payload->Data;
            if (dragged && dragged != go && !go->isDescendantOf(dragged)) {
                reparent(dragged, go);
                openNodes_.insert(go);
                auto sp = findShared(dragged);
                if (sp) setSelection(sp);
            }
        }
        EndDragDropTarget();
    }

    if (open) openNodes_.insert(go); else openNodes_.erase(go);

    if (open) {
        for (auto* c : go->children) {
            drawHierarchyNode(c);
        }
        TreePop();
    }

    PushID((void*)((uintptr_t)go ^ 0xBCDE));
    InvisibleButton("##drop-below", ImVec2(avail.x, 3.0f));
    if (BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = AcceptDragDropPayload("GO_PTR")) {
            GameObject* dragged = *(GameObject* const*)payload->Data;
            if (dragged && dragged != go) {
                if (go->parent && dragged->parent == go->parent) {
                    int idx = go->indexInParent();
                    reorderSibling(dragged, go->parent, idx + 1);
                }
                else if (!go->parent && !dragged->parent) {
                    int newIndex = 0;
                    if (scene_) {
                        std::vector<GameObject*> roots;
                        for (auto& sp : *scene_) if (sp->parent == nullptr) roots.push_back(sp.get());
                        for (int i = 0; i < (int)roots.size(); ++i) if (roots[i] == go) { newIndex = i + 1; break; }
                    }
                    reorderRoot(dragged, newIndex);
                }
                auto sp = findShared(dragged);
                if (sp) setSelection(sp);
            }
        }
        EndDragDropTarget();
    }
    PopID();

    if (pendingFocus_ == go) {
        ImGui::SetScrollHereY(0.25f);
        pendingFocus_ = nullptr;
    }
}


void EditorWindows::ensureChecker() {
    if (checker_tex_) return;
    const int W = 128, H = 128; checker_w_ = W; checker_h_ = H;
    std::vector<unsigned int> rgba(W * H, 0xffffffffu);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            bool blk = ((x / 8) + (y / 8)) % 2 == 0;
            rgba[y * W + x] = blk ? 0xff202020u : 0xffffffffu;
        }
    glGenTextures(1, &checker_tex_);
    glBindTexture(GL_TEXTURE_2D, checker_tex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}


void EditorWindows::drawInspector() {
    ImGui::SetNextWindowSize(ImVec2(360, 520), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Inspector", &show_inspector_)) { ImGui::End(); return; }

    // Sección GameObject (solo si hay selección)
    if (selected_ && *selected_) {
        auto go = *selected_;
        ImGui::Text("Name: %s", go->name.c_str());
        ImGui::Separator();
        auto& T = go->transform;
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto pos = T.pos();
            float p[3] = { (float)pos.x, (float)pos.y, (float)pos.z };
            if (ImGui::DragFloat3("Position", p, 0.1f)) T.setPosition({ p[0], p[1], p[2] });
            ImGui::SameLine(); if (ImGui::Button("Reset##pos")) T.setPosition({ 0,0,0 });

            static float rot[3] = { 0,0,0 };
            static float last[3] = { 0,0,0 };
            if (ImGui::DragFloat3("Rotation", rot, 0.2f)) {
                float d[3] = { rot[0] - last[0], rot[1] - last[1], rot[2] - last[2] };
                T.rotateEulerDeltaDeg({ d[0], d[1], d[2] });
                memcpy(last, rot, sizeof(rot));
            }
            ImGui::SameLine(); if (ImGui::Button("Reset##rot")) { T.resetRotation(); memset(rot, 0, sizeof(rot)); memset(last, 0, sizeof(last)); }
            auto sc = T.getScale();
            float s[3] = { (float)sc.x, (float)sc.y, (float)sc.z };
            if (ImGui::DragFloat3("Scale", s, 0.05f, 0.001f, 1000.0f)) T.setScale({ s[0], s[1], s[2] });
            ImGui::SameLine(); if (ImGui::Button("Reset##scale")) T.resetScale();
            auto L = T.left(), U = T.up(), F = T.fwd();
        }

        if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Drop Target for Mesh
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "[Drop Model Here]");
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
                    std::string path = (const char*)payload->Data;
                    // Check extension
                    std::string ext = path.substr(path.find_last_of("."));
                    if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb") {
                         auto meshes = ModelLoader::loadModel(path);
                         if (!meshes.empty()) {
                             go->setMesh(meshes[0]);
                             LOG_INFO("Dropped Mesh: " + path);
                         } else {
                             LOG_ERROR("Failed to load dropped mesh: " + path);
                         }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (go->mesh) {
                ImGui::Text("Vertices: %zu", go->mesh->getVertexCount());
                ImGui::Text("Triangles: %zu", go->mesh->getTriangleCount());
                ImGui::Separator();
                ImGui::Checkbox("Show Vertex Normals", &go->mesh->showVertexNormals);
                ImGui::Checkbox("Show Face Normals", &go->mesh->showFaceNormals);
                if (go->mesh->showVertexNormals || go->mesh->showFaceNormals) {
                    float normalLen = (float)go->mesh->normalLength;
                    if (ImGui::SliderFloat("Normal Length", &normalLen, 0.01f, 2.0f))
                        go->mesh->normalLength = normalLen;
                }
            }
            else ImGui::TextDisabled("No mesh attached.");
        }
        if (ImGui::CollapsingHeader("Texture", ImGuiTreeNodeFlags_DefaultOpen)) {
            unsigned int texID = go->getTextureID();
            int tw = 0, th = 0; if (texID) glTextureSize(texID, tw, th);
            ImGui::Text("Texture ID: %u", texID);
            ImGui::Text("Size: %dx%d", tw, th);
            ImGui::Separator();
            ensureChecker();
            ImGui::BeginDisabled(texID == checker_tex_);
            if (ImGui::Button("Apply Checkerboard")) {
                if (prev_tex_.find(go.get()) == prev_tex_.end())
                    prev_tex_[go.get()] = texID;
                go->setTexture(checker_tex_);
                LOG_INFO("Applied checkerboard texture to " + go->name);
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            bool canRestore = prev_tex_.find(go.get()) != prev_tex_.end();
            ImGui::BeginDisabled(!canRestore);
            if (ImGui::Button("Restore Texture")) {
                go->setTexture(prev_tex_[go.get()]);
                prev_tex_.erase(go.get());
                LOG_INFO("Restored original texture for " + go->name);
            }
            ImGui::EndDisabled();
            if (texID != 0 && tw > 0 && th > 0) {
                ImGui::Separator();
                ImGui::Text("Preview:");
                float previewSize = 200.0f;
                float ar = (float)tw / (float)th;
                ImVec2 size = ar > 1.0f ? ImVec2(previewSize, previewSize / ar) : ImVec2(previewSize * ar, previewSize);
                ImGui::Image((ImTextureID)(intptr_t)texID, size);
            }
            
            // Drop Target for Texture
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "[Drop Texture Here]");
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
                    std::string path = (const char*)payload->Data;
                    // Check extension
                    std::string ext = path.substr(path.find_last_of("."));
                    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".dds" || ext == ".bmp") {
                         GLuint newTex = ModelLoader::loadTexture(path);
                         if (newTex) {
                             if (prev_tex_.find(go.get()) == prev_tex_.end())
                                prev_tex_[go.get()] = go->getTextureID(); // Save old for restore
                             go->setTexture(newTex);
                             LOG_INFO("Dropped Texture: " + path);
                         } else {
                             LOG_ERROR("Failed to load dropped texture: " + path);
                         }
                    }
                }
                ImGui::EndDragDropTarget();
            }
        }
        // Camera Component
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Camera Component", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Is Camera", &go->camera.enabled);
            if (go->camera.enabled) {
                float fovDeg = glm::degrees((float)go->camera.fov);
                if (ImGui::SliderFloat("FOV", &fovDeg, 30.0f, 120.0f)) go->camera.fov = glm::radians(fovDeg);
                
                float zNear = (float)go->camera.zNear;
                if (ImGui::DragFloat("Near", &zNear, 0.01f, 0.01f, 100.0f)) go->camera.zNear = zNear;
                
                float zFar = (float)go->camera.zFar;
                if (ImGui::DragFloat("Far", &zFar, 1.0f, 10.0f, 10000.0f)) go->camera.zFar = zFar;
                
                ImGui::TextDisabled("Aspect Ratio is controlled by Window size");
            }
        }
    }
    else {
        ImGui::TextDisabled("No GameObject selected.");
    }
    ImGui::End();
}


std::string EditorWindows::getAssetsPath() {
    if (asset_database_) return asset_database_->getAssetsPath();
    auto try_find = [](fs::path start)->std::string {
        fs::path p = start;
        for (int i = 0; i < 8; ++i) {
            fs::path c = p / "Assets";
            if (fs::exists(c) && fs::is_directory(c)) return fs::absolute(c).string();
            if (!p.has_parent_path()) break;
            p = p.parent_path();
        }
        return "";
    };
    if (auto s = try_find(fs::current_path()); !s.empty()) return s;
    if (const char* base = SDL_GetBasePath()) {
        if (auto s = try_find(fs::path(base)); !s.empty()) return s;
    }
    return fs::absolute(fs::path("Assets")).string();
}

void EditorWindows::loadPrimitiveFromAssets(const std::string& name) {
    if (!scene_)return;
    std::string assets = getAssetsPath();
    fs::path p = fs::absolute(fs::path(assets) / "Primitives" / name);
    auto meshes = ModelLoader::loadModel(p.string());
    if (meshes.empty()) { LOG_ERROR("Failed to load primitive: " + p.string()); return; }
    for (size_t i = 0; i < meshes.size(); ++i) {
        auto go = std::make_shared<GameObject>(name + "_" + std::to_string(i));
        go->setMesh(meshes[i]);
        scene_->push_back(go);
    }
    LOG_INFO("Loaded primitive: " + p.string());
}



/*-------------------------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------*/
void EditorWindows::setSelection(const std::shared_ptr<GameObject>& go) {
    if (!selected_) return;
    if (*selected_) (*selected_)->isSelected = false;
    *selected_ = go;
    if (*selected_) (*selected_)->isSelected = true;
}

void EditorWindows::collectPostorder(GameObject* root, std::vector<GameObject*>& out) {
    if (!root) return;
    for (auto* c : root->children) collectPostorder(c, out);
    out.push_back(root);
}

std::shared_ptr<GameObject> EditorWindows::findShared(GameObject* raw) {
    if (!scene_ || !raw) return {};
    for (auto& sp : *scene_) if (sp.get() == raw) return sp;
    return {};
}

void EditorWindows::removeFromScene(GameObject* raw) {
    if (!scene_ || !raw) return;
    auto& v = *scene_;
    for (auto it = v.begin(); it != v.end(); ++it) {
        if (it->get() == raw) { v.erase(it); break; }
    }
}

void EditorWindows::deleteSelectedRecursive() {
    if (!selected_ || !(*selected_)) return;
    GameObject* S = selected_->get();
    *selected_ = nullptr;

    std::vector<GameObject*> post;
    collectPostorder(S, post);

    for (auto* n : post) {
        if (n->parent) n->parent->removeChild(n);
    }
    for (auto* n : post) {
        openNodes_.erase(n);
        removeFromScene(n);
    }
    pendingDelete_ = nullptr;
}

void EditorWindows::reparent(GameObject* dragged, GameObject* target) {
    if (!dragged || dragged == target) return;
    if (target && (target == dragged || target->isDescendantOf(dragged))) return;

    mat4 M_old(1.0);
    if (preserve_world_) M_old = computeWorldMatrix(dragged);

    if (dragged->parent) dragged->parent->removeChild(dragged);

    if (target) target->addChild(dragged);
    else dragged->parent = nullptr;

    if (preserve_world_) {
        setLocalFromWorld(dragged, M_old, target);
    }
}

void EditorWindows::reorderSibling(GameObject* node, GameObject* parent, int newIndex) {
    if (!node || !parent) return;
    auto& v = parent->children;
    int cur = node->indexInParent();
    if (cur < 0) return;
    if (newIndex > (int)v.size()) newIndex = (int)v.size();
    if (newIndex < 0) newIndex = 0;
    if (newIndex == cur || newIndex == cur + 1) return;
    auto it = std::find(v.begin(), v.end(), node);
    if (it == v.end()) return;
    v.erase(it);
    if (newIndex > cur) newIndex -= 1;
    v.insert(v.begin() + newIndex, node);
}

void EditorWindows::reorderRoot(GameObject* node, int newIndex) {
    if (!scene_ || !node || node->parent) return;
    std::vector<int> rootSceneIdx;
    std::vector<std::shared_ptr<GameObject>> rootSPs;
 for (int i = 0; i < (int)scene_->size(); ++i) {
        if ((*scene_)[i]->parent == nullptr) {
   rootSceneIdx.push_back(i);
            rootSPs.push_back((*scene_)[i]);
   }
    }
    int curRootIdx = -1;
    for (int i = 0; i < (int)rootSPs.size(); ++i) if (rootSPs[i].get() == node) { curRootIdx = i; break; }
    if (curRootIdx < 0) return;

    if (newIndex < 0) newIndex = 0;
    if (newIndex > (int)rootSPs.size()) newIndex = (int)rootSPs.size();
    auto spNode = rootSPs[curRootIdx];
    int sceneIdx = rootSceneIdx[curRootIdx];
    (*scene_).erase((*scene_).begin() + sceneIdx);

    rootSceneIdx.clear(); rootSPs.clear();
    for (int i = 0; i < (int)scene_->size(); ++i) {
        if ((*scene_)[i]->parent == nullptr) {
  rootSceneIdx.push_back(i);
       rootSPs.push_back((*scene_)[i]);
        }
    }
    int destSceneIdx;
    if (newIndex >= (int)rootSPs.size()) {
        destSceneIdx = rootSceneIdx.empty() ? (int)scene_->size() : (rootSceneIdx.back() + 1);
    }
    else {
        destSceneIdx = rootSceneIdx[newIndex];
    }
    (*scene_).insert((*scene_).begin() + destSceneIdx, spNode);
}

/*-------------------------------------------------------------------------------------------------------*/
/*---------------------------------------ASSETS WINDOW------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------*/

void EditorWindows::drawAssets() {
    ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Assets", &show_assets_)) { ImGui::End(); return; }
    {
        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetWindowSize();
        assetsViewBounds_.x = pos.x;
        assetsViewBounds_.y = pos.y;
        assetsViewBounds_.w = size.x;
        assetsViewBounds_.h = size.y;
        assetsViewBounds_.isHovered = ImGui::IsWindowHovered();
        assetsViewBounds_.isFocused = ImGui::IsWindowFocused();
    }

    // Info box
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Drag and drop files here to import");
    ImGui::Separator();

    // Toolbar
    if (ImGui::Button("Refresh##assets")) {
 if (asset_database_) {
    asset_database_->refresh();
            LOG_INFO("Assets refreshed from disk");
  }
    }
    ImGui::SameLine();
    
    // Show total assets
    int assetCount = 0;
    if (asset_database_) {
    assetCount = asset_database_->getAllAssets().size();
    }
    ImGui::Text("Assets: %d", assetCount);

    ImGui::SameLine();
    ImGui::Checkbox("Show References##assetdb", &show_asset_refs_);

    ImGui::Separator();

    // Asset tree
    ImGui::BeginChild("AssetTree", ImVec2(0, 0), true);
    
    if (asset_database_) {
        std::string assetsPath = asset_database_->getAssetsPath();
        if (fs::exists(assetsPath)) {
 ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
            bool rootOpen = ImGui::TreeNodeEx("Assets", flags);
        
// Accept drag-drop for folder
            if (ImGui::BeginDragDropTarget()) {
           if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("") ) {
   // File drop handling
       }
      ImGui::EndDragDropTarget();
            }

            if (rootOpen) {
                drawAssetTree(assetsPath, 0);
                ImGui::TreePop();
            }
        } else {
    ImGui::TextDisabled("Assets folder not found");
        }
    } else {
      ImGui::TextDisabled("AssetDatabase not initialized");
    }

    ImGui::EndChild();
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !selected_asset_.empty()) {
            ImGui::OpenPopup("Confirm Asset Delete");
        }
    }
    if (ImGui::BeginPopupModal("Confirm Asset Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        AssetMeta* meta = asset_database_ ? asset_database_->findAssetBySourcePath(selected_asset_) : nullptr;
        ImGui::Text("Delete '%s'?", selected_asset_.c_str());
        if (meta) {
            ImGui::Text("Type: %s", meta->assetType.c_str());
            ImGui::Text("GUID: %.8s", meta->guid.c_str());
            if (show_asset_refs_) ImGui::Text("References: %d", meta->referenceCount);
        }
        bool canDelete = !(meta && meta->referenceCount > 0);
        ImGui::BeginDisabled(!canDelete);
        if (ImGui::Button("Delete", ImVec2(120, 0))) {
            if (asset_database_ && asset_database_->deleteAsset(selected_asset_)) {
                selected_asset_.clear();
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        if (!canDelete) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1,0.8f,0,1), "Cannot delete: asset has active references.");
        }
        ImGui::EndPopup();
    }
    if (!selected_asset_.empty()) {
        ImGui::Separator();
        ImGui::Text("Selected: %s", selected_asset_.c_str());
        AssetMeta* meta = asset_database_ ? asset_database_->findAssetBySourcePath(selected_asset_) : nullptr;
        if (meta) {
            ImGui::Text("Type: %s", meta->assetType.c_str());
            ImGui::Text("GUID: %.8s", meta->guid.c_str());
            if (show_asset_refs_) ImGui::Text("References: %d", meta->referenceCount);
        }
    }
    ImGui::End();
}

void EditorWindows::drawAssetTree(const std::string& folderPath, int depth) {
    if (depth > 5) return;  // Limit recursion depth
   if (!asset_database_) return;

    try {
        for (const auto& entry : fs::directory_iterator(folderPath)) {
            std::string name = entry.path().filename().string();
         
            // Skip .meta files and hidden files
            if (name.empty() || name[0] == '.') continue;
            if (name.size() >= 5 && name.substr(name.size() - 5) == ".meta") continue;

      if (entry.is_directory()) {
    // Folder node
    bool isOpen = expanded_asset_folders_.count(entry.path().string()) > 0;
   ImGui::SetNextItemOpen(isOpen);
        
            if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ImGui::IsItemHovered()) hoveredAssetFolder_ = entry.path().string();
                if (ImGui::IsItemClicked()) {
                  // Toggle expanded state
        std::string fullPath = entry.path().string();
       if (isOpen) {
           expanded_asset_folders_.erase(fullPath);
     } else {
 expanded_asset_folders_.insert(fullPath);
          }
                }
           
                drawAssetTree(entry.path().string(), depth + 1);
                ImGui::TreePop();
            }
        } else {
            // Asset file
            std::string fullPath = entry.path().string();
            if (!assetsFilter_.PassFilter(name.c_str())) { continue; }
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                
        // Highlight if selected
          bool isSelected = (selected_asset_ == fullPath);
            if (isSelected) {
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
           }

            ImGui::TreeNodeEx(name.c_str(), flags);

            if (isSelected) {
                ImGui::PopStyleColor();
            }
        
           // Selection and interaction
            if (ImGui::IsItemClicked()) {
                selected_asset_ = fullPath;
            }

            if (ImGui::BeginDragDropSource()) {
                ImGui::SetDragDropPayload("ASSET_PATH", fullPath.c_str(), (int)fullPath.size() + 1);
                ImGui::TextUnformatted(name.c_str());
                ImGui::EndDragDropSource();
            }

            // Context menu for asset
            if (ImGui::BeginPopupContextItem()) {
                AssetMeta* meta = asset_database_->findAssetBySourcePath(fullPath);

                if (meta) {
      ImGui::Text("Type: %s", meta->assetType.c_str());
        ImGui::Text("GUID: %.8s", meta->guid.c_str());
            if (show_asset_refs_) {
      ImGui::Text("References: %d", meta->referenceCount);
   }
           ImGui::Separator();
       }

                    if (ImGui::MenuItem("Show in Explorer")) {
            // Platform-specific file explorer open
                   #ifdef _WIN32
        system(("explorer /select,\"" + fullPath + "\"").c_str());
      #endif
    }
                    if (ImGui::MenuItem("Copy Path")) {
                        ImGui::SetClipboardText(fullPath.c_str());
                    }

        if (ImGui::MenuItem("Delete")) {
              if (meta && meta->referenceCount > 0) {
          LOG_WARN("Cannot delete asset with active references (" + std::to_string(meta->referenceCount) + ")");
       } else {
                if (asset_database_->deleteAsset(fullPath)) {
       selected_asset_ = "";
        }
             }
        }

ImGui::EndPopup();
      }
  }
   }
 } catch (const std::exception& e) {
        ImGui::TextDisabled("Error reading folder");
   LOG_ERROR("Asset tree error: " + std::string(e.what()));
    }
}
void EditorWindows::drawGameWindow(unsigned int texID, int w, int h, bool isPlaying) {
    if (ImGui::Begin("Game View", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        
        // Render Image
        ImGui::Image((ImTextureID)(intptr_t)texID, viewportSize, ImVec2(0, 1), ImVec2(1, 0));
        
        // Capture Bounds for Input
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        
        gameViewBounds_.x = min.x;
        gameViewBounds_.y = min.y;
        gameViewBounds_.w = max.x - min.x;
        gameViewBounds_.h = max.y - min.y;
        gameViewBounds_.isHovered = ImGui::IsItemHovered();
        gameViewBounds_.isFocused = ImGui::IsWindowFocused(); 
    }
    ImGui::End();
}

void EditorWindows::drawSceneWindow(unsigned int texID, int w, int h) {
    if (ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        ImGui::Image((ImTextureID)(intptr_t)texID, viewportSize, ImVec2(0, 1), ImVec2(1, 0));
        
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        
        sceneViewBounds_.x = min.x;
        sceneViewBounds_.y = min.y;
        sceneViewBounds_.w = max.x - min.x;
        sceneViewBounds_.h = max.y - min.y;
        sceneViewBounds_.isHovered = ImGui::IsItemHovered();
        sceneViewBounds_.isFocused = ImGui::IsWindowFocused(); 
    }
    ImGui::End();
}


const EditorWindows::ViewportBounds& EditorWindows::getAssetsViewBounds() const {
    return assetsViewBounds_;
}
const std::string& EditorWindows::getHoveredAssetFolder() const {
    return hoveredAssetFolder_;
}
