#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include "Camera.h"
#include "GameObject.h"
#include "ModelLoader.h"
#include <chrono>
#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include "EditorWindows.h"
#include "Logger.h"
#include "AssetDatabase.h"
#include <imgui_impl_sdl3.h>
#include <filesystem>
#include "AABB.h"
#include "Frustum.h"
#include "Octree.h"
#include "SceneSerializer.h"
#include "Framebuffer.h" // [NEW]

using namespace std;
namespace fs = std::filesystem;
static EditorWindows editor; // Restored

static Camera editorCamera;
static bool editorCameraInitialized = false;

// [NEW] Octree instance
static Octree mainOctree(AABB(vec3(-200, -200, -200), vec3(200, 200, 200)));
// [NEW] Viewport Framebuffers
static Framebuffer sceneFramebuffer;
static Framebuffer gameFramebuffer;

static std::shared_ptr<GameObject> mainCamera = nullptr;
struct CameraState { Transform transform; GameObject::CameraComponent camComp; };
static CameraState storedEditorCamera; // To restore editor camera properties
static auto lastFrameTime = chrono::high_resolution_clock::now();
SDL_Window* window = nullptr;
static SDL_GLContext glContext = nullptr;
static bool running = true;
static vector<shared_ptr<GameObject>> gameObjects;
static shared_ptr<GameObject> selectedGameObject = nullptr;

// [NEW] Simulation State
static bool isPlaying = false;
static bool isPaused = false;
static bool s_Step = false; // Restored

// static Camera editorCamera; // REMOVED
static string tempScenePath = "temp.scene";

static std::string getAssetsPath() {
    namespace fs = std::filesystem;
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

static std::string getLibraryPathPortable(const std::string& assetsPath) {
    namespace fs = std::filesystem;
    std::string pref = SDL_GetPrefPath("Motor", "Motor");
    if (!pref.empty()) {
        fs::path p = fs::path(pref) / "Library";
        return fs::absolute(p).string();
    }
    fs::path p = fs::path(assetsPath) / "Library";
    return fs::absolute(p).string();
}

static void loadModelToScene(const std::string& filename, const std::string& namePrefix) {
    std::string assetsPath = getAssetsPath();
    fs::path modelPath = fs::absolute(fs::path(assetsPath) / filename);
    
    if (!fs::exists(modelPath)) {
        std::cerr << "[ERROR] " << filename << " not found at: " << modelPath.string() << std::endl;
        return;
    }

    auto meshes = ModelLoader::loadModel(modelPath.string());
    if (meshes.empty()) {
        std::cerr << "[ERROR] Failed to load " << filename << std::endl;
        return;
    }

    std::cout << "[Scene] Loaded " << filename << " with " << meshes.size() << " meshes." << std::endl;

    for (size_t i = 0; i < meshes.size(); i++) {
        auto gameObject = std::make_shared<GameObject>(namePrefix + "_" + std::to_string(i));
        gameObject->setMesh(meshes[i]);
        
        // [NEW] Store metadata for serialization
        gameObject->modelPath = filename;
        gameObject->meshIndex = (int)i;
        
        // Optional: Position BakerHouse slightly differently so it's not inside the street or vice versa?
        // For now, load at origin as requested.
        
        gameObjects.push_back(gameObject);
        mainOctree.insert(gameObject);
    }
}



static void createMainCamera() {
    // Check if camera already exists in scene
    for (auto& go : gameObjects) {
        if (go->camera.enabled) {
            mainCamera = go;
            return;
        }
    }

    // Create new
    mainCamera = std::make_shared<GameObject>("Main Camera");
    mainCamera->camera.enabled = true;
    mainCamera->transform.setPosition({0, 5, 20});
    mainCamera->transform.rotateEulerDeltaDeg({ -15, 0, 0 }); 
    
    gameObjects.push_back(mainCamera);
    mainOctree.insert(mainCamera);
}

static void loadDefaultScene() {
    mainOctree.clear();
    gameObjects.clear(); // Clear existing objects if any
    
    std::cout << "Loading Default Scene..." << std::endl;
    loadModelToScene("street.fbx", "Street");
    loadModelToScene("BakerHouse.fbx", "BakerHouse");
    
    createMainCamera();
}

static void loadModelFromFile(const string& filepath) {
    cout << "Loading model from: " << filepath << endl;
    auto meshes = ModelLoader::loadModel(filepath);
    if (meshes.empty()) {
        cerr << "ERROR: Failed to load model from: " << filepath << endl;
        return;
    }
    cout << "Model loaded successfully with " << meshes.size() << " meshes." << endl;
    size_t lastSlash = filepath.find_last_of("/\\");
    size_t lastDot = filepath.find_last_of(".");
    string modelName = filepath.substr(lastSlash + 1, lastDot - lastSlash - 1);
    for (size_t i = 0; i < meshes.size(); i++) {
        auto gameObject = make_shared<GameObject>(modelName + "_" + to_string(i));
        gameObject->setMesh(meshes[i]);
        gameObject->transform.pos() = vec3(gameObjects.size() * 2.0, 0, 0);
        gameObjects.push_back(gameObject);
        mainOctree.insert(gameObject); // [NEW] Insert into Octree
    }
}

static void loadTextureFromFile(const string& filepath) {
    cout << "Loading texture from: " << filepath << endl;
    GLuint textureID = ModelLoader::loadTexture(filepath);
    if (textureID == 0) {
        cerr << "ERROR: Failed to load texture from: " << filepath << endl;
        return;
    }
    cout << "Texture loaded successfully (ID: " << textureID << ")" << endl;
    int appliedCount = 0;
    for (auto& go : gameObjects) {
        if (go->isSelected) {
            go->setTexture(textureID);
            appliedCount++;
        }
    }
    if (appliedCount == 0)
        cout << "No GameObjects selected. Please select one first." << endl;
    else
        cout << "Texture applied to " << appliedCount << " selected GameObject(s)." << endl;
}

static void handleDropFile(const string& filepath) {
    size_t dotPos = filepath.find_last_of(".");
    if (dotPos == string::npos) {
        cout << "No file extension found" << endl;
        return;
    }
    string extension = filepath.substr(dotPos);
    transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (extension == ".fbx") loadModelFromFile(filepath);
    else if (extension == ".dds" || extension == ".png" || extension == ".jpg" || extension == ".jpeg")
        loadTextureFromFile(filepath);
    else {
        cout << "Unsupported file type: " << extension << endl;
        cout << "Supported: FBX, DDS, PNG, JPG." << endl;
    }
}

static std::string copyIntoAssets(const std::string& srcPath, const std::string& targetFolder = "") {
    namespace fs = std::filesystem;
    try {
        fs::path src(srcPath);
        if (!fs::exists(src) || !fs::is_regular_file(src)) return "";
        std::string assets = AssetDatabase::instance().getAssetsPath();
        fs::path base = targetFolder.empty() ? fs::path(assets) : fs::path(targetFolder);
        if (!fs::exists(base)) base = fs::path(assets);
        fs::path dest = base / src.filename();
        if (fs::exists(dest)) {
            std::string stem = dest.stem().string();
            std::string ext = dest.has_extension() ? dest.extension().string() : "";
            int i = 1;
            while (fs::exists(dest)) {
                dest = base / (stem + "_" + std::to_string(i) + ext);
                ++i;
            }
        }
        fs::copy_file(src, dest, fs::copy_options::overwrite_existing);
        return fs::absolute(dest).string();
    } catch (...) {
        return "";
    }
}

static void draw_triangle(const glm::u8vec3& color, const vec3& center, double size) {
    glColor3ub(color.r, color.g, color.b);
    glBegin(GL_TRIANGLES);
    glVertex3d(center.x, center.y + size, center.z);
    glVertex3d(center.x - size, center.y - size, center.z);
    glVertex3d(center.x + size, center.y - size, center.z);
    glEnd();
}

static void draw_floorGrid(int size, double step) {
    glColor3ub(100, 100, 100);
    glBegin(GL_LINES);
    for (double i = -size; i <= size; i += step) {
        glVertex3d(i, 0, -size);
        glVertex3d(i, 0, size);
        glVertex3d(-size, 0, i);
        glVertex3d(size, 0, i);
    }
    glEnd();
}

static void drawAABB(const AABB& box, const glm::u8vec3& color) {
    glDisable(GL_LIGHTING);
    glColor3ub(color.r, color.g, color.b);
    glLineWidth(2.0f);
    vec3 min = box.min;
    vec3 max = box.max;
    glBegin(GL_LINES);
    glVertex3d(min.x, min.y, min.z); glVertex3d(max.x, min.y, min.z);
    glVertex3d(max.x, min.y, min.z); glVertex3d(max.x, min.y, max.z);
    glVertex3d(max.x, min.y, max.z); glVertex3d(min.x, min.y, max.z);
    glVertex3d(min.x, min.y, max.z); glVertex3d(min.x, min.y, min.z);
    glVertex3d(min.x, max.y, min.z); glVertex3d(max.x, max.y, min.z);
    glVertex3d(max.x, max.y, min.z); glVertex3d(max.x, max.y, max.z);
    glVertex3d(max.x, max.y, max.z); glVertex3d(min.x, max.y, max.z);
    glVertex3d(min.x, max.y, max.z); glVertex3d(min.x, max.y, min.z);
    glVertex3d(min.x, min.y, min.z); glVertex3d(min.x, max.y, min.z);
    glVertex3d(max.x, min.y, min.z); glVertex3d(max.x, max.y, min.z);
    glVertex3d(max.x, min.y, max.z); glVertex3d(max.x, max.y, max.z);
    glVertex3d(min.x, min.y, max.z); glVertex3d(min.x, max.y, max.z);
    glEnd();
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}

static void drawFrustum(const Frustum& frustum, const mat4& invProjView, const glm::u8vec3& color) {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glColor3ub(color.r, color.g, color.b);
    glLineWidth(2.0f);

    // Obtener las 8 esquinas del frustum
    vec3 corners[8];
    frustum.getCorners(corners, invProjView);

    glBegin(GL_LINES);

    // Near plane
    glVertex3d(corners[0].x, corners[0].y, corners[0].z);
    glVertex3d(corners[1].x, corners[1].y, corners[1].z);

    glVertex3d(corners[1].x, corners[1].y, corners[1].z);
    glVertex3d(corners[3].x, corners[3].y, corners[3].z);

    glVertex3d(corners[3].x, corners[3].y, corners[3].z);
    glVertex3d(corners[2].x, corners[2].y, corners[2].z);

    glVertex3d(corners[2].x, corners[2].y, corners[2].z);
    glVertex3d(corners[0].x, corners[0].y, corners[0].z);

    // Far plane (4 líneas)
    glVertex3d(corners[4].x, corners[4].y, corners[4].z);
    glVertex3d(corners[5].x, corners[5].y, corners[5].z);

    glVertex3d(corners[5].x, corners[5].y, corners[5].z);
    glVertex3d(corners[7].x, corners[7].y, corners[7].z);

    glVertex3d(corners[7].x, corners[7].y, corners[7].z);
    glVertex3d(corners[6].x, corners[6].y, corners[6].z);

    glVertex3d(corners[6].x, corners[6].y, corners[6].z);
    glVertex3d(corners[4].x, corners[4].y, corners[4].z);

    // Connecting lines
    glVertex3d(corners[0].x, corners[0].y, corners[0].z);
    glVertex3d(corners[4].x, corners[4].y, corners[4].z);

    glVertex3d(corners[1].x, corners[1].y, corners[1].z);
    glVertex3d(corners[5].x, corners[5].y, corners[5].z);

    glVertex3d(corners[2].x, corners[2].y, corners[2].z);
    glVertex3d(corners[6].x, corners[6].y, corners[6].z);

    glVertex3d(corners[3].x, corners[3].y, corners[3].z);
    glVertex3d(corners[7].x, corners[7].y, corners[7].z);

    glEnd();
    glLineWidth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

static void updateProjection(int width, int height) {
    double targetAspect = 16.0 / 9.0;
    int viewportWidth = width, viewportHeight = height, viewportX = 0, viewportY = 0;
    double currentAspect = static_cast<double>(width) / height;
    if (currentAspect > targetAspect) {
        viewportWidth = static_cast<int>(height * targetAspect);
        viewportX = (width - viewportWidth) / 2;
    }
    else {
        viewportHeight = static_cast<int>(width / targetAspect);
        viewportY = (height - viewportHeight) / 2;
    }
    glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
    
    if (mainCamera && mainCamera->camera.enabled) {
         mainCamera->camera.aspect = targetAspect;
         
         glMatrixMode(GL_PROJECTION);
         // Construct Projection
         double fov = mainCamera->camera.fov;
         double zNear = mainCamera->camera.zNear;
         double zFar = mainCamera->camera.zFar;
         mat4 projection = glm::perspective(fov, targetAspect, zNear, zFar);
         glLoadMatrixd(glm::value_ptr(projection));
    }
}

static Ray getRayFromMouse(int mouseX, int mouseY, const vec3& camPos, const mat4& proj, const mat4& view, int screenWidth, int screenHeight) {
    double x = (2.0 * mouseX) / screenWidth - 1.0;
    double y = 1.0 - (2.0 * mouseY) / screenHeight; 

    vec4 rayClip(x, y, -1.0, 1.0);
    mat4 invProj = glm::inverse(proj);
    vec4 rayEye = invProj * rayClip;
    rayEye = vec4(rayEye.x, rayEye.y, -1.0, 0.0); 

    mat4 invView = glm::inverse(view);
    vec4 rayWorld = invView * rayEye;
    vec3 direction = glm::normalize(vec3(rayWorld));

    return Ray(camPos, direction);
}

static shared_ptr<GameObject> selectWithRaycast(int mouseX, int mouseY) {
    if (gameObjects.empty()) return nullptr;
    int screenWidth, screenHeight;
    SDL_GetWindowSize(window, &screenWidth, &screenHeight);
    
    Ray ray(vec3(0), vec3(0,0,-1));
    if(mainCamera && mainCamera->camera.enabled) {
        double fov = mainCamera->camera.fov;
        double aspect = mainCamera->camera.aspect;
        double zNear = mainCamera->camera.zNear;
        double zFar = mainCamera->camera.zFar;
        mat4 proj = glm::perspective(fov, aspect, zNear, zFar);
        mat4 view = glm::inverse(mainCamera->transform.mat());
        ray = getRayFromMouse(mouseX, mouseY, mainCamera->transform.pos(), proj, view, screenWidth, screenHeight);
    }
    
    // [NEW] Use Octree to get candidates
    // Note: If Objects moved, we should ideally update octree.
    // For now, assuming static scene or that user will accept some inaccuracies if moving things wildly.
    auto candidates = mainOctree.queryRay(ray);
    
    // If octree is empty or something fails, fallback to all? 
    // No, empty candidates means no intersection with large boxes.
    // However, since we might dynamically add objects without inserting (if I missed a spot), 
    // let's stick to candidates.
    
    // Fallback: If candidates empty but gameObjects not empty, maybe Octree not populated?
    // Should be populated.
    
    shared_ptr<GameObject> closest = nullptr;
    double closestDist = std::numeric_limits<double>::max();

    for (auto& go : candidates) {
        if (!go->mesh) continue;
        mat4 worldMatrix = computeWorldMatrix(go.get());
        AABB worldAABB = go->mesh->getWorldAABB(worldMatrix);
        double t;
        if (ray.intersectsAABB(worldAABB, t)) {
            if (t < closestDist) {
                closestDist = t;
                closest = go;
            }
        }
    }
    return closest;
}


static void handle_input(double deltaTime) {
    SDL_Event event;
    
    // Bounds
    const auto& sceneBounds = editor.getSceneViewBounds();
    const auto& assetsBounds = editor.getAssetsViewBounds();
    
    bool allowEditorInput = !isPlaying; 

    // SDL3: Mouse coordinates are floats
    float mx, my;
    SDL_GetMouseState(&mx, &my);
    bool insideScene = (mx >= sceneBounds.x && mx <= sceneBounds.x + sceneBounds.w &&
         my >= sceneBounds.y && my <= sceneBounds.y + sceneBounds.h);
    bool insideAssets = (mx >= assetsBounds.x && mx <= assetsBounds.x + assetsBounds.w &&
         my >= assetsBounds.y && my <= assetsBounds.y + assetsBounds.h);
    
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT) running = false;
        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window)) running = false;
        if (event.type == SDL_EVENT_DROP_FILE && event.drop.data) {
            string droppedFile = event.drop.data;
            if (insideAssets) {
                std::string target = editor.getHoveredAssetFolder();
                std::string copied = copyIntoAssets(droppedFile, target);
                if (!copied.empty()) {
                    AssetDatabase::instance().importAsset(copied);
                    AssetDatabase::instance().refresh();
                    LOG_INFO(std::string("Imported into Assets: ") + copied);
                } else {
                    LOG_ERROR(std::string("Failed to import into Assets: ") + droppedFile);
                }
            } else {
                handleDropFile(droppedFile);
            }
        }
    
        // EDITOR CAMERA INPUT - Solo si estamos en la ventana de Scene y permitido
        if (allowEditorInput && insideScene) {
            // Mouse button events - Solo si estamos en Scene
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                int state = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) ? 1 : 0;
                editorCamera.onMouseButton(event.button.button, state, event.button.x, event.button.y);
            }
            // Mouse motion - Procesar siempre si RMB está presionado (relative mode lo maneja)
            else if (event.type == SDL_EVENT_MOUSE_MOTION) {
                editorCamera.onMouseMove(event.motion.x, event.motion.y);
            }
            // Mouse wheel
            else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                editorCamera.onMouseWheel(event.wheel.y);
            }
            // Keyboard
            else if (event.type == SDL_EVENT_KEY_DOWN) {
                editorCamera.onKeyDown(event.key.scancode);

                // F key for focus
                if (event.key.key == SDLK_F && selectedGameObject) {
                    mat4 m = computeWorldMatrix(selectedGameObject.get());
                    vec3 target = vec3(m[3]); // Position
                    double radius = 1.0;
                    if (selectedGameObject->mesh && selectedGameObject->mesh->localAABB.isValid()) {
                        vec3 extent = selectedGameObject->mesh->localAABB.max - selectedGameObject->mesh->localAABB.min;
                        radius = glm::length(extent) * 0.5;
                    }

                    editorCamera.focusOn(target, radius);
                }
            }
            else if (event.type == SDL_EVENT_KEY_UP) {
                editorCamera.onKeyUp(event.key.scancode);
            }
        }
        // Si RMB está presionado pero saliste de Scene, dejar de procesar rotación
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_RIGHT) {
            // Soltar RMB incluso fuera de Scene
            editorCamera.onMouseButton(event.button.button, 0, event.button.x, event.button.y);
        }
 
        // Selection raycast (left click in scene) - Solo permitido si estamos en Scene
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT && insideScene && !ImGui::GetIO().WantCaptureMouse) {
            float localX = event.button.x - sceneBounds.x;
            float localY = event.button.y - sceneBounds.y;
  
            mat4 view = editorCamera.view();
            double aspect = (sceneBounds.h > 0) ? sceneBounds.w / sceneBounds.h : 1.0;
            mat4 proj = editorCamera.projection();
   
            Ray ray = getRayFromMouse((int)localX, (int)localY, editorCamera.transform.pos(), proj, view, (int)sceneBounds.w, (int)sceneBounds.h);

            auto candidates = mainOctree.queryRay(ray);
            shared_ptr<GameObject> closest = nullptr;
            double closestDist = std::numeric_limits<double>::max();
       
            for (auto& go : candidates) {
            if (!go->mesh) continue;
                mat4 worldMatrix = computeWorldMatrix(go.get());
                AABB worldAABB = go->mesh->getWorldAABB(worldMatrix);
                double t;
                if (ray.intersectsAABB(worldAABB, t)) {
                    if (t < closestDist) { closestDist = t; closest = go; }
                }
            }
  
            for(auto& g : gameObjects) g->isSelected = false;
            if (closest) {
                closest->isSelected = true;
                selectedGameObject = closest;
            } else {
                selectedGameObject = nullptr;
            }
        }
    }

    // Update camera - Solo si en Scene
    if (allowEditorInput && insideScene) {
      editorCamera.update(deltaTime);
    }
}

static void render() {
    auto currentTime = chrono::high_resolution_clock::now();
    double deltaTime = chrono::duration<double>(currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;

    int winW, winH;
    SDL_GetWindowSize(window, &winW, &winH);
    
    // ============================================
    // 1. SCENE VIEW (Editor Camera)
    // ============================================
    // Init Editor Camera first time
    if (!editorCameraInitialized) {
         editorCamera.transform.setPosition({0, 5, 20});
         editorCamera.fov = glm::radians(60.0);
         
         editorCameraInitialized = true;
    }
    
    // Resize Scene FBO
    auto sceneBounds = editor.getSceneViewBounds();
    if (sceneBounds.w > 0 && sceneBounds.h > 0) {
  if (sceneBounds.w != sceneFramebuffer.GetWidth() || sceneBounds.h != sceneFramebuffer.GetHeight()) 
        sceneFramebuffer.Rescale(sceneBounds.w, sceneBounds.h);
    } else {
        if (!sceneFramebuffer.GetTextureID()) sceneFramebuffer.Init(winW, winH);
    }
    
    sceneFramebuffer.Bind();
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f); // Dark Gray Grid Bg
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    
    // Setup Editor View using Camera class
    mat4 viewEditor = editorCamera.view();
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd(glm::value_ptr(viewEditor));
    
    // Update aspect ratio based on scene framebuffer
    double aspect = (sceneFramebuffer.GetHeight() > 0) ? (double)sceneFramebuffer.GetWidth() / (double)sceneFramebuffer.GetHeight() : 1.77;
    editorCamera.aspect = aspect;
    mat4 projEditor = editorCamera.projection();
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(glm::value_ptr(projEditor));
    glMatrixMode(GL_MODELVIEW);
    
    draw_floorGrid(20, 1.0);
    
    // Draw Objects (Editor Pass)
    Frustum editorFrustum;
    editorFrustum.extractFromCamera(projEditor * viewEditor);
    
    if (editor.isFrustumCullingEnabled()) {
        auto potentials = mainOctree.queryFrustum(editorFrustum);
        for (const auto& go : potentials) if (go->mesh) go->draw(); 
    } else {
        for (const auto& go : gameObjects) go->draw();
    }
    
    // Draw Debug Gizmos (AABBs, Frustums, etc.) - ONLY IN SCENE VIEW
    if (editor.shouldShowAABBs()) {
        mainOctree.drawDebug();
        for (const auto& go : gameObjects) {
            if (go->mesh && go->mesh->localAABB.isValid()) {
                mat4 m = computeWorldMatrix(go.get());
                AABB worldAABB = go->mesh->getWorldAABB(m);
                glm::u8vec3 color = go->isSelected ? glm::u8vec3(255, 255, 0) : glm::u8vec3(255, 0, 0);
                drawAABB(worldAABB, color);
            }
        }
    }
    
    sceneFramebuffer.Unbind();
    
    
    // ============================================
    // 2. GAME VIEW (Main Camera)
    // ============================================
    auto gameBounds = editor.getGameViewBounds();
    if (gameBounds.w > 0 && gameBounds.h > 0) {
        if (gameBounds.w != gameFramebuffer.GetWidth() || gameBounds.h != gameFramebuffer.GetHeight())
            gameFramebuffer.Rescale(gameBounds.w, gameBounds.h);
    } else {
       if (!gameFramebuffer.GetTextureID()) gameFramebuffer.Init(winW, winH);
    }

    gameFramebuffer.Bind();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black for Game
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (mainCamera && mainCamera->camera.enabled) {
        glEnable(GL_DEPTH_TEST);
        
        mat4 viewGame = glm::inverse(mainCamera->transform.mat());
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixd(glm::value_ptr(viewGame));
        
        double aspect = (gameFramebuffer.GetHeight() > 0) ? (double)gameFramebuffer.GetWidth() / (double)gameFramebuffer.GetHeight() : 1.77;
        mat4 projGame = glm::perspective(glm::radians((double)mainCamera->camera.fov), aspect, mainCamera->camera.zNear, mainCamera->camera.zFar);
      
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixd(glm::value_ptr(projGame));
        glMatrixMode(GL_MODELVIEW);
      
     // Draw Objects (Game Pass)
        Frustum gameFrustum;
        gameFrustum.extractFromCamera(projGame * viewGame);
        
        if (editor.isFrustumCullingEnabled()) {
            auto potentials = mainOctree.queryFrustum(gameFrustum);
            for (const auto& go : potentials) if (go->mesh) go->draw(); 
        } else {
            for (const auto& go : gameObjects) go->draw();
        }
    }
    gameFramebuffer.Unbind();
    
    
    // ============================================
    // 3. FINAL COMPOSITION
    // ============================================
    glDisable(GL_SCISSOR_TEST); 
    glViewport(0, 0, winW, winH); 
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT); 
    
    // Send Both Textures to UI
    editor.setSceneViewTexture(sceneFramebuffer.GetTextureID(), sceneFramebuffer.GetWidth(), sceneFramebuffer.GetHeight());
    editor.setGameViewTexture(gameFramebuffer.GetTextureID(), gameFramebuffer.GetWidth(), gameFramebuffer.GetHeight());
    
    static bool lastPlaying = false;
    editor.render(&isPlaying, &isPaused, &s_Step);

    if (isPlaying && !lastPlaying) {
        // Start
        cout << "Saving scene to " << tempScenePath << endl;
        SceneSerializer::SaveScene(tempScenePath, gameObjects);
        // Save Editor Camera
        if (mainCamera) {
            storedEditorCamera.transform = mainCamera->transform;
            storedEditorCamera.camComp = mainCamera->camera;
        }
    }
    else if (!isPlaying && lastPlaying) {
        // Stop
        cout << "Stopping Simulation... Restoring scene..." << endl;
        SceneSerializer::LoadScene(tempScenePath, gameObjects);
        
        mainCamera = nullptr;
        for(auto& go : gameObjects) if(go->camera.enabled) mainCamera = go;
        
        if (mainCamera) {
            mainCamera->transform = storedEditorCamera.transform;
            mainCamera->camera = storedEditorCamera.camComp;
        }
        mainOctree.clear();
        for(auto& go : gameObjects) mainOctree.insert(go);
        selectedGameObject = nullptr;
    }
    lastPlaying = isPlaying;
    
    if (isPlaying && (!isPaused || s_Step)) {
    s_Step = false;
    }

    SDL_GL_SwapWindow(window);
}

static void init_opengl() {
    glewInit();
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    GLfloat lightPos[] = { 5.0f, 10.0f, 5.0f, 1.0f };
    GLfloat lightAmbient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat lightDiffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        cout << "SDL could not be initialized! " << SDL_GetError() << endl;
        return EXIT_FAILURE;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    int screenWidth = 1280, screenHeight = 720;
    window = SDL_CreateWindow("Motor", screenWidth, screenHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        cout << "Window could not be created! " << SDL_GetError() << endl;
        return EXIT_FAILURE;
    }
    glContext = SDL_GL_CreateContext(window);
    editor.init(window, glContext);
    editor.setScene(&gameObjects, &selectedGameObject);
    editor.setScene(&gameObjects, &selectedGameObject);
    if (!glContext) {
        cout << "OpenGL context could not be created!" << endl;
        return EXIT_FAILURE;
    }
    init_opengl();
    
    // Initialize AssetDatabase
    std::string assetsPath = getAssetsPath();
    std::string libraryPath = getLibraryPathPortable(assetsPath);
    AssetDatabase::instance().initialize(assetsPath, libraryPath);
    editor.setAssetDatabase(&AssetDatabase::instance());  // Connect to editor

    //camera.transform.pos() = vec3(0, 5, 10);
    //camera.transform.pos() = vec3(0, 5, 10);
    // Initial Projection Update
    updateProjection(screenWidth, screenHeight);
    // Init scene logic moved to loadDefaultScene
    loadDefaultScene();
    cout << "========================================" << endl;
    cout << "Instructions:" << endl;
    cout << "- Drag & drop FBX to load models" << endl;
    cout << "- Drag & drop PNG/JPG/DDS to apply textures" << endl;
    cout << "- Left click to select/cycle objects" << endl;
    cout << "- Right drag: rotate camera, WASD: move, wheel: zoom" << endl;
    cout << "========================================" << endl;
    auto lastTime = std::chrono::high_resolution_clock::now();
    while (running) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        double deltaTime = std::chrono::duration<double>(currentTime - lastTime).count();
        lastTime = currentTime;

        handle_input(deltaTime);
        render();
        if (editor.wantsQuit()) running = false;
        SDL_Delay(1);
    }
    gameObjects.clear();
    AssetDatabase::instance().shutdown();
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    editor.shutdown();
    SDL_Quit();
    return EXIT_SUCCESS;
}
