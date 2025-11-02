#include "EditorWindows.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"   // ⭐ 改用 OpenGL3 后端
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <filesystem>
#include <vector>
#include <algorithm>
#include "ModelLoader.h"

using std::string;
namespace fs = std::filesystem;

// ========== 内部工具 ==========
static void glTextureSize(GLuint tex, int& w, int& h) {
    w = h = 0;
    if (!tex) return;
    glBindTexture(GL_TEXTURE_2D, tex);
    GLint tw = 0, th = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tw);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &th);
    w = tw; h = th;
}

// ========== 初始化/销毁/场景指针 ==========
void EditorWindows::init(SDL_Window* window, SDL_GLContext gl) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // SDL + OpenGL3 后端初始化；GLSL 版本建议 130（OpenGL 3.0）
    ImGui_ImplSDL3_InitForOpenGL(window, gl);
    ImGui_ImplOpenGL3_Init("#version 130");

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

// ========== 每帧入口 ==========
void EditorWindows::render() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // FPS 统计
    ImGuiIO& io = ImGui::GetIO();
    fps_history_[fps_index_] = io.Framerate;
    fps_index_ = (fps_index_ + 1) % kFpsHistory;

    drawMainMenu();

    if (show_console_)   drawConsole();
    if (show_config_)    drawConfig();
    if (show_hierarchy_) drawHierarchy();
    if (show_inspector_) drawInspector();

    if (show_about_) {
        if (ImGui::Begin("About", &show_about_)) {
            ImGui::TextUnformatted("Nombre del motor: Motor - FBX Loader");
            ImGui::TextUnformatted("Version: 0.1.0");
            ImGui::Separator();
            ImGui::TextUnformatted("Miembros del equipo: (rellenar)");
            ImGui::TextUnformatted("Librerias: SDL3, OpenGL, GLEW, ImGui, Assimp, DevIL");
            ImGui::TextUnformatted("Licencia: MIT");
            ImGui::End();
        }
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// ========== 菜单栏 ==========
void EditorWindows::drawMainMenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit", "Esc")) {
                wants_quit_ = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Console", nullptr, &show_console_);
            ImGui::MenuItem("Config", nullptr, &show_config_);
            ImGui::MenuItem("Hierarchy", nullptr, &show_hierarchy_);
            ImGui::MenuItem("Inspector", nullptr, &show_inspector_);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Primitives")) {
            if (ImGui::MenuItem("Load Cube (Assets/Primitives/Cube.fbx)")) {
                loadPrimitiveFromAssets("Cube.fbx");
            }
            if (ImGui::MenuItem("Load Sphere (Assets/Primitives/Sphere.fbx)")) {
                loadPrimitiveFromAssets("Sphere.fbx");
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            static const char* REPO = "https://github.com/your_org/your_repo";
            if (ImGui::MenuItem("GitHub Docs"))       SDL_OpenURL((std::string(REPO) + "/docs").c_str());
            if (ImGui::MenuItem("Report a bug"))      SDL_OpenURL((std::string(REPO) + "/issues").c_str());
            if (ImGui::MenuItem("Download latest"))   SDL_OpenURL((std::string(REPO) + "/releases").c_str());
            ImGui::Separator();
            ImGui::MenuItem("About", nullptr, &show_about_);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

// ========== Console 窗口 ==========
void EditorWindows::drawConsole() {
    ImGui::SetNextWindowSize(ImVec2(600, 220), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Console", &show_console_)) { ImGui::End(); return; }

    static ImGuiTextFilter filter;
    static bool auto_scroll = true;
    filter.Draw("Filter");

    ImGui::Separator();
    ImGui::BeginChild("scroll_region", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    auto logs = Logger::instance().snapshot();
    for (auto& e : logs) {
        if (!filter.PassFilter(e.text.c_str())) continue;
        ImVec4 color = ImVec4(1, 1, 1, 1);
        if (e.level == LogLevel::Warning) color = ImVec4(1, 1, 0, 1);
        if (e.level == LogLevel::Error)   color = ImVec4(1, 0.5f, 0.5f, 1);
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(e.text.c_str());
        ImGui::PopStyleColor();
    }
    if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();

    ImGui::Checkbox("Auto-scroll", &auto_scroll);
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        Logger::instance().setMaxEntries(0);
        Logger::instance().setMaxEntries(1000);
    }

    ImGui::End();
}

// ========== Config 窗口 ==========
void EditorWindows::drawConfig() {
    ImGui::SetNextWindowSize(ImVec2(420, 380), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Config", &show_config_)) { ImGui::End(); return; }

    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::PlotLines("Framerate", fps_history_, kFpsHistory,
        fps_index_, nullptr, 0.0f, 200.0f, ImVec2(-1, 80));

    ImGui::Separator();
    ImGui::Text("OpenGL : %s", (const char*)glGetString(GL_VERSION));
    ImGui::Text("GPU    : %s", (const char*)glGetString(GL_RENDERER));
    ImGui::Text("Vendor : %s", (const char*)glGetString(GL_VENDOR));

    ImGui::Separator();
    ImGui::Text("SDL CPU cores : %d", SDL_GetNumLogicalCPUCores());
    ImGui::Text("SDL SystemRAM : %d MB", SDL_GetSystemRAM());

    ImGui::End();
}

// ========== Hierarchy 窗口 ==========
void EditorWindows::drawHierarchy() {
    ImGui::SetNextWindowSize(ImVec2(260, 420), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Hierarchy", &show_hierarchy_)) { ImGui::End(); return; }
    if (!scene_) { ImGui::TextDisabled("No scene."); ImGui::End(); return; }

    for (size_t i = 0; i < scene_->size(); ++i) {
        auto& go = (*scene_)[i];
        bool selected = selected_ && *selected_ && go.get() == selected_->get();
        if (ImGui::Selectable(go->name.c_str(), selected)) {
            if (selected_ && *selected_) (*selected_)->isSelected = false;
            if (selected_) *selected_ = go;
            go->isSelected = true;
        }
    }
    ImGui::End();
}

// ========== Inspector 窗口 ==========
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
    ImGui::SetNextWindowSize(ImVec2(360, 420), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Inspector", &show_inspector_)) { ImGui::End(); return; }
    if (!selected_ || !(*selected_)) { ImGui::TextDisabled("No selection."); ImGui::End(); return; }

    auto go = *selected_;
    ImGui::Text("Name: %s", go->name.c_str());
    ImGui::Separator();

    // Transform（只读展示；后续可加编辑）
    auto& T = go->transform;
    auto P = T.pos();
    ImGui::Text("Position: (%.3f, %.3f, %.3f)", P.x, P.y, P.z);

    ImGui::Separator();
    ImGui::Text("Mesh");
    ImGui::TextDisabled("(vertex/triangle counts: add here if Mesh exposes them)");

    // Texture 信息 + 棋盘格
    ImGui::Separator();
    ImGui::Text("Texture");

    unsigned int texID = 0;
#ifdef __has_include
#if __has_include("GameObject.h")
    texID = go->getTextureID(); // 需要 GameObject/Mesh 提供 getter（见我之前的补丁）
#endif
#endif

    int tw = 0, th = 0;
    if (texID) glTextureSize(texID, tw, th);
    ImGui::Text("ID: %u, Size: %dx%d", texID, tw, th);

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

    ImGui::End();
}

// ========== 资源路径与基础几何 ==========
std::string EditorWindows::getAssetsPath() {
    auto cwd = fs::current_path();

    auto try_find = [](fs::path start)->std::string {
        fs::path p = start;
        for (int i = 0; i < 6; ++i) {
            fs::path c = p / "Assets";
            if (fs::exists(c) && fs::is_directory(c)) return c.string();
            if (!p.has_parent_path()) break;
            p = p.parent_path();
        }
        return "";
        };

    if (auto s = try_find(cwd); !s.empty()) return s;
    const char* base = SDL_GetBasePath();
    if (base) if (auto s = try_find(fs::path(base)); !s.empty()) return s;
    return "Assets";
}

void EditorWindows::loadPrimitiveFromAssets(const std::string& name) {
    if (!scene_) return;
    std::string assets = getAssetsPath();
    fs::path p = fs::absolute(fs::path(assets) / "Primitives" / name);
    auto meshes = ModelLoader::loadModel(p.string());
    if (meshes.empty()) {
        LOG_ERROR(std::string("Failed to load primitive: ") + p.string());
        return;
    }
    for (size_t i = 0; i < meshes.size(); ++i) {
        auto go = std::make_shared<GameObject>(name + "_" + std::to_string(i));
        go->setMesh(meshes[i]);
        scene_->push_back(go);
    }
    LOG_INFO(std::string("Loaded primitive: ") + p.string());
}
