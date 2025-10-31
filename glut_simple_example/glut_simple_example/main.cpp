#include <iostream>
#include <glm/glm.hpp>
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

using namespace std;

static Camera camera;
static auto lastFrameTime = chrono::high_resolution_clock::now();
SDL_Window* window = nullptr;
static SDL_GLContext glContext = nullptr;
static bool running = true;

// Game objects storage
static vector<shared_ptr<GameObject>> gameObjects;
static shared_ptr<GameObject> selectedGameObject = nullptr;

static string getAssetsPath() {
    
    string paths[] = {
        "Assets",
        "../Assets",
        "../../Assets"
    };

    for (const auto& p : paths) {
        
        return p;
    }

    return "Assets"; 
}

static void loadBakerHouse() {
    string assetsPath = getAssetsPath();
    string bakerHousePath = assetsPath + "/BakerHouse.fbx";

    cout << "Attempting to load BakerHouse from: " << bakerHousePath << endl;

    auto meshes = ModelLoader::loadModel(bakerHousePath);

    if (meshes.empty()) {
        cerr << "ERROR: Failed to load BakerHouse model!" << endl;
        cerr << "Make sure BakerHouse.fbx is in the Assets folder." << endl;
        return;
    }

    cout << "BakerHouse loaded successfully with " << meshes.size() << " meshes." << endl;

    for (size_t i = 0; i < meshes.size(); i++) {
        auto gameObject = make_shared<GameObject>("BakerHouse_" + to_string(i));
        gameObject->setMesh(meshes[i]);
        gameObjects.push_back(gameObject);
    }
}

static void loadModelFromFile(const string& filepath) {
    cout << "Loading model from: " << filepath << endl;

    auto meshes = ModelLoader::loadModel(filepath);

    if (meshes.empty()) {
        cerr << "ERROR: Failed to load model from: " << filepath << endl;
        return;
    }

    cout << "Model loaded successfully with " << meshes.size() << " meshes." << endl;

    // Extract filename for naming
    size_t lastSlash = filepath.find_last_of("/\\");
    size_t lastDot = filepath.find_last_of(".");
    string modelName = filepath.substr(lastSlash + 1, lastDot - lastSlash - 1);

    // Create GameObjects for each mesh
    for (size_t i = 0; i < meshes.size(); i++) {
        auto gameObject = make_shared<GameObject>(modelName + "_" + to_string(i));
        gameObject->setMesh(meshes[i]);

        // Position new models slightly offset from origin
        gameObject->transform.pos() = vec3(gameObjects.size() * 2.0, 0, 0);

        gameObjects.push_back(gameObject);
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

    // Apply texture to selected GameObjects
    int appliedCount = 0;
    for (auto& go : gameObjects) {
        if (go->isSelected) {
            go->setTexture(textureID);
            appliedCount++;
        }
    }

    if (appliedCount == 0) {
        cout << "No GameObjects selected. Texture not applied." << endl;
        cout << "Please select a GameObject first (click on it)." << endl;
    }
    else {
        cout << "Texture applied to " << appliedCount << " selected GameObject(s)." << endl;
    }
}

static void handleDropFile(const string& filepath) {
    // Get file extension
    size_t dotPos = filepath.find_last_of(".");
    if (dotPos == string::npos) {
        cout << "No file extension found" << endl;
        return;
    }

    string extension = filepath.substr(dotPos);

    transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (extension == ".fbx") {
        loadModelFromFile(filepath);
    }
    else if (extension == ".dds" || extension == ".png" || extension == ".jpg" || extension == ".jpeg") {
        loadTextureFromFile(filepath);
    }
    else {
        cout << "Unsupported file type: " << extension << endl;
        cout << "Supported formats: FBX for models, DDS/PNG/JPG for textures." << endl;
    }
}

static void draw_triangle(const glm::u8vec3& color, const vec3& center, double size)
{
    glColor3ub(color.r, color.g, color.b);
    glBegin(GL_TRIANGLES);
    glVertex3d(center.x, center.y + size, center.z);
    glVertex3d(center.x - size, center.y - size, center.z);
    glVertex3d(center.x + size, center.y - size, center.z);
    glEnd();
}

static void draw_floorGrid(int size, double step)
{
    glColor3ub(100, 100, 100);
    glBegin(GL_LINES);
    for (double i = -size; i <= size; i += step)
    {
        glVertex3d(i, 0, -size);
        glVertex3d(i, 0, size);
        glVertex3d(-size, 0, i);
        glVertex3d(size, 0, i);
    }
    glEnd();
}

static void updateProjection(int width, int height)
{
    double targetAspect = 16.0 / 9.0;
    int viewportWidth = width;
    int viewportHeight = height;
    int viewportX = 0;
    int viewportY = 0;

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
    camera.aspect = targetAspect;
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(&camera.projection()[0][0]);
}

static void handle_input() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_QUIT:
            running = false;
            break;

        case SDL_EVENT_KEY_DOWN:
            if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                running = false;
            }
            camera.onKeyDown(event.key.scancode);
            break;

        case SDL_EVENT_KEY_UP:
            camera.onKeyUp(event.key.scancode);
            break;

        case SDL_EVENT_MOUSE_WHEEL:
            camera.onMouseWheel(event.wheel.y);
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            camera.onMouseButton(event.button.button, 1, event.button.x, event.button.y);

            if (event.button.button == SDL_BUTTON_LEFT && !gameObjects.empty()) {
                if (selectedGameObject) {
                    selectedGameObject->isSelected = false;
                }
                static size_t selectionIndex = 0;
                selectionIndex = (selectionIndex + 1) % gameObjects.size();
                selectedGameObject = gameObjects[selectionIndex];
                selectedGameObject->isSelected = true;
                cout << "Selected: " << selectedGameObject->name << endl;
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            camera.onMouseButton(event.button.button, 0, event.button.x, event.button.y);
            break;

        case SDL_EVENT_MOUSE_MOTION:
            camera.onMouseMove(event.motion.x, event.motion.y);
            break;

        case SDL_EVENT_WINDOW_RESIZED: {
            int width, height;
            SDL_GetWindowSize(window, &width, &height);
            updateProjection(width, height);
            break;
        }

        case SDL_EVENT_DROP_BEGIN:
            cout << "Drop begin" << endl;
            break;

        case SDL_EVENT_DROP_FILE: {
            if (event.drop.data != nullptr) {
                string droppedFile = event.drop.data;
                cout << "File dropped: " << droppedFile << endl;
                cout << "  Position: (" << event.drop.x << ", " << event.drop.y << ")" << endl;
                if (event.drop.source != nullptr) {
                    cout << "  Source: " << event.drop.source << endl;
                }
                handleDropFile(droppedFile);
            }
            break;
        }

        case SDL_EVENT_DROP_TEXT: {
            if (event.drop.data != nullptr) {
                cout << "Text dropped: " << event.drop.data << endl;
            }
            break;
        }

        case SDL_EVENT_DROP_POSITION:
            break;

        case SDL_EVENT_DROP_COMPLETE:
            cout << "Drop complete" << endl;
            break;
        }
    }
}

static void render() {
    auto currentTime = chrono::high_resolution_clock::now();
    double deltaTime = chrono::duration<double>(currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;

    camera.update(deltaTime);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd(&camera.view()[0][0]);

    // Draw floor grid
    draw_floorGrid(16, 0.25);

    // Draw all game objects
    glColor3f(1.0f, 1.0f, 1.0f);
    for (const auto& gameObject : gameObjects) {
        gameObject->draw();
    }

    // Draw debug triangles
    draw_triangle(Colors::Red, vec3(-1, 0.25, 0), 0.5);
    draw_triangle(Colors::Green, vec3(0, 0.5, 0.25), 0.5);
    draw_triangle(Colors::Blue, vec3(1, -0.5, -0.25), 0.5);

    SDL_GL_SwapWindow(window);
}

static void init_opengl() {
    glewInit();

    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

    glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glClearColor(0.5, 0.5, 0.5, 1.0);

    // Enable lighting for better model visualization
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

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        cout << "SDL could not be initialized! SDL_Error: " << SDL_GetError() << endl;
        return EXIT_FAILURE;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    int screenWidth = 1280;
    int screenHeight = 720;

    // Create main window
    window = SDL_CreateWindow("Motor - FBX Loader", screenWidth, screenHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!window) {
        cout << "Window could not be created! SDL_Error: " << SDL_GetError() << endl;
        return EXIT_FAILURE;
    }

    // Set up OpenGL
    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        cout << "OpenGL context could not be created! SDL_Error: " << SDL_GetError() << endl;
        return EXIT_FAILURE;
    }

    init_opengl();

    // Set up camera starting position
    camera.transform().pos() = vec3(0, 5, 10);
    camera.orbitTarget = vec3(0, 0, 0);

    updateProjection(screenWidth, screenHeight);

    // Load BakerHouse automatically
    cout << "========================================" << endl;
    cout << "Loading BakerHouse..." << endl;
    cout << "========================================" << endl;
    loadBakerHouse();

    cout << "========================================" << endl;
    cout << "Instructions:" << endl;
    cout << "- Drag & drop FBX files to load models" << endl;
    cout << "- Drag & drop DDS/PNG/JPG to apply textures" << endl;
    cout << "- Left click to cycle through objects" << endl;
    cout << "- Right mouse drag: rotate camera" << endl;
    cout << "- WASD: move camera (when right mouse held)" << endl;
    cout << "- Mouse wheel: zoom" << endl;
    cout << "- F: focus on orbit target" << endl;
    cout << "- ESC: quit" << endl;
    cout << "========================================" << endl;

    // Main game loop
    while (running) {
        handle_input();
        render();
        SDL_Delay(1);
    }

    // Clean up
    gameObjects.clear();
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}