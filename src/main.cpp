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
#include "SceneSerializer.h" // [NEW]

using namespace std;
namespace fs = std::filesystem;
static EditorWindows editor; // Restored


// static Camera camera; // REMOVED
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

// [NEW] Octree instance (Size -200 to 200 to cover typical scenes)
static Octree mainOctree(AABB(vec3(-200, -200, -200), vec3(200, 200, 200)));

static std::string getAssetsPath() {
    namespace fs = std::filesystem;
    auto cwd = fs::current_path();
    const char* base = SDL_GetBasePath();
    std::string baseStr = base ? base : "";
    std::cout << "[Path] CWD        = " << cwd.string() << std::endl;
    std::cout << "[Path] SDL base   = " << baseStr << std::endl;
    auto try_find = [](fs::path start)->std::string {
        fs::path p = start;
        for (int i = 0; i < 6; ++i) {
            fs::path candidate = p / "Assets";
            if (fs::exists(candidate) && fs::is_directory(candidate)) {
                std::cout << "[Path] Found Assets at: " << candidate.string() << std::endl;
                return candidate.string();
            }
            if (!p.has_parent_path()) break;
            p = p.parent_path();
        }
        return "";
        };
    if (auto s = try_find(cwd); !s.empty()) return s;
    if (!baseStr.empty()) {
        if (auto s = try_find(fs::path(baseStr)); !s.empty()) return s;
    }

    std::cout << "[Path] Assets NOT found; fallback to literal 'Assets'." << std::endl;
    return "Assets";
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



// [MOVED] Input Handling
static void handle_input() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT) running = false;
        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window)) running = false;
        
        // Handle Camera Mouse Look
        if (mainCamera && !ImGui::GetIO().WantCaptureMouse) {
             const bool* keys = SDL_GetKeyboardState(NULL);
             if (event.type == SDL_EVENT_MOUSE_MOTION && (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT))) {
                 float sensitivity = 0.2f;
                 float dx = (float)event.motion.xrel;
                 float dy = (float)event.motion.yrel;
                 // Rotate Pitch (X) and Yaw (Y/WorldUp)
                 // Note: RotateEulerDeltaDeg is local.
                 mainCamera->transform.rotateEulerDeltaDeg(vec3(-dy * sensitivity, -dx * sensitivity, 0));
             }
        }
        
        if (event.type == SDL_EVENT_DROP_FILE && event.drop.data) {
             string droppedFile = event.drop.data;
             handleDropFile(droppedFile);
        }
    }
    
    // Continuous Keyboard
    if (mainCamera && !ImGui::GetIO().WantCaptureKeyboard) {
        const bool* keys = SDL_GetKeyboardState(NULL);
        if ((SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT)) || isPlaying) {
            double speed = 0.1; 
            if (keys[SDL_SCANCODE_LSHIFT]) speed *= 3.0;
            
            vec3 move(0);
            if (keys[SDL_SCANCODE_W]) move += mainCamera->transform.fwd();
            if (keys[SDL_SCANCODE_S]) move -= mainCamera->transform.fwd();
            if (keys[SDL_SCANCODE_A]) move -= mainCamera->transform.right();
            if (keys[SDL_SCANCODE_D]) move += mainCamera->transform.right();
            
            if (glm::length(move) > 0.0001) {
                mainCamera->transform.translate(glm::normalize(move) * speed);
            }
        }
    }
}

static void render() {
    auto currentTime = chrono::high_resolution_clock::now();
    double deltaTime = chrono::duration<double>(currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;
    // camera.update(deltaTime); // Removed

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    
    mat4 view(1.0f);
    mat4 projection(1.0f);
    
    if (mainCamera && mainCamera->camera.enabled) {
        view = glm::inverse(mainCamera->transform.mat());
        glLoadMatrixd(glm::value_ptr(view));
        
        // Construct Projection for Frustum/GL
        double aspect = mainCamera->camera.aspect; // Should be updated by window size
        double fov = mainCamera->camera.fov;
        double zNear = mainCamera->camera.zNear;
        double zFar = mainCamera->camera.zFar;
        projection = glm::perspective(fov, aspect, zNear, zFar);
    } else {
        glLoadIdentity();
    }

    draw_floorGrid(16, 0.25);
    
    mat4 projView = projection * view;
    Frustum frustum;
    frustum.extractFromCamera(projView);
    int totalObjects = (int)gameObjects.size();
    int culledObjects = 0;
    int renderedObjects = 0;
    glColor3f(1.0f, 1.0f, 1.0f);
    
    // [NEW] Use Octree for rendering if Culling enabled
    if (editor.isFrustumCullingEnabled()) {
        auto potentials = mainOctree.queryFrustum(frustum);
        // Still double check exact AABB for accuracy, or trust node containment?
        // Query returns objects in intersecting nodes. Precise check is good.
        for (const auto& go : potentials) {
             if (go->mesh) {
                mat4 worldMatrix = computeWorldMatrix(go.get());
                AABB worldAABB = go->mesh->getWorldAABB(worldMatrix);
                if (frustum.containsAABB(worldAABB)) {
                    go->draw();
                    renderedObjects++;
                } else {
                    // candidate from octree, but accurate cull failed
                    // culledObjects count is tricky here because we don't iterate all O(N)
                    // We can estimate culled = Total - Rendered
                }
             }
        }
        culledObjects = totalObjects - renderedObjects;
    } 
    else {
        // [OLD] Linear path
        for (const auto& go : gameObjects) {
             go->draw();
             renderedObjects++;
        }
    }

    // Dibujar AABBs si está activado
    if (editor.shouldShowAABBs()) {
        // [NEW] Draw Octree Debug
        mainOctree.drawDebug();
        
        // Also draw object AABBs for verification
        for (const auto& go : gameObjects) {
            if (go->mesh && go->mesh->localAABB.isValid()) {
                mat4 worldMatrix = computeWorldMatrix(go.get());
                AABB worldAABB = go->mesh->getWorldAABB(worldMatrix);
                glm::u8vec3 color;
                if (go->isSelected) {
                    color = glm::u8vec3(255, 255, 0); 
                }
                else {
                    bool inFrustum = frustum.containsAABB(worldAABB);
                    color = inFrustum ? glm::u8vec3(0, 255, 0) : glm::u8vec3(255, 0, 0);
                }
                drawAABB(worldAABB, color);
            }
        }
    }
    // Dibujar frustum si está activado
    if (editor.shouldShowFrustum()) {
        mat4 invProjView = glm::inverse(projView);
        drawFrustum(frustum, invProjView, glm::u8vec3(255, 255, 255));
    }

    // [NEW] Draw Toolbar and Handle State (Handled inside editor.render)
    static bool lastPlaying = false;
    
    // Pass state to editor so it draws the toolbar INSIDE the ImGui frame
    editor.render(&isPlaying, &isPaused, &s_Step);

    if (isPlaying && !lastPlaying) {
        // Start
        // Start
        cout << "Starting Simulation... Saving scene to " << tempScenePath << endl;
        SceneSerializer::SaveScene(tempScenePath, gameObjects);
        // Save Editor Camera
        if (mainCamera) {
            storedEditorCamera.transform = mainCamera->transform;
            storedEditorCamera.camComp = mainCamera->camera;
        }
    }
    else if (!isPlaying && lastPlaying) {
        // Stop
        // Stop
        cout << "Stopping Simulation... Restoring scene..." << endl;
        SceneSerializer::LoadScene(tempScenePath, gameObjects);
        
        // Find main camera again
        mainCamera = nullptr;
        for(auto& go : gameObjects) if(go->camera.enabled) mainCamera = go;
        
        // Restore Editor Camera if found
        if (mainCamera) {
            mainCamera->transform = storedEditorCamera.transform;
            mainCamera->camera = storedEditorCamera.camComp;
        }
        
        // Re-insert into Octree (since objects are new)
        mainOctree.clear();
        for(auto& go : gameObjects) mainOctree.insert(go);
        
        selectedGameObject = nullptr; // Selection invalid
    }
    lastPlaying = isPlaying;
    
    // Simulation Logic (if playing and not paused, or stepping)
    if (isPlaying && (!isPaused || s_Step)) {
        // Here we would update physics or scripts
        // For now, nothing moves unless scripts exist
        s_Step = false;
    }

    // editor.render() called above

    static int frameCount = 0;
    if (editor.isFrustumCullingEnabled() && ++frameCount >= 60) {
        frameCount = 0;
         cout << "Culling Stats - Total: " << totalObjects 
              << " Rendered: " << renderedObjects 
              << " Culled: " << culledObjects << endl;
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
    window = SDL_CreateWindow("Motor", screenWidth, screenHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        cout << "Window could not be created! " << SDL_GetError() << endl;
        return EXIT_FAILURE;
    }
    glContext = SDL_GL_CreateContext(window);
    editor.init(window, glContext);
    editor.setScene(&gameObjects, &selectedGameObject);
    editor.setScene(&gameObjects, &selectedGameObject);
    // editor.setMainCamera(&camera);  // REMOVED
    if (!glContext) {
        cout << "OpenGL context could not be created!" << endl;
        return EXIT_FAILURE;
    }
    init_opengl();
    
    // Initialize AssetDatabase
    std::string assetsPath = getAssetsPath();
    std::string libraryPath = fs::absolute(fs::path(assetsPath).parent_path() / "Library").string();
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
    while (running) {
        handle_input();
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