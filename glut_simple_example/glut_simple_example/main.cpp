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
#include "EditorWindows.h"
#include <filesystem> // ⭐ 新增：自动检测 Assets 用

using namespace std;

static EditorWindows editor;
static Camera camera;
static auto lastFrameTime = chrono::high_resolution_clock::now();
SDL_Window* window = nullptr;
static SDL_GLContext glContext = nullptr;
static bool running = true;

// Game objects storage
static vector<shared_ptr<GameObject>> gameObjects;
static shared_ptr<GameObject> selectedGameObject = nullptr;


// 自动检测最近的 Assets 文件夹（无需手动改路径）Detectar automaticamente la carpeta de Activos mas reciente

static std::string getAssetsPath() {
    namespace fs = std::filesystem;

    // 日志输出：当前工作目录和 SDL 基路径Salida: Directorio de trabajo actual y ruta base de SDL
    auto cwd = fs::current_path();
    /*char* base = SDL_GetBasePath();
    std::string baseStr = base ? base : "";
    if (base) SDL_free(base);*/
    const char* base = SDL_GetBasePath();
    std::string baseStr = base ? base : "";
    

    std::cout << "[Path] CWD        = " << cwd.string() << std::endl;
    std::cout << "[Path] SDL base   = " << baseStr << std::endl;

    // 向上查找6层目录，找到最近的 Assets Busca en los 6 niveles superiores del directorio para encontrar los recursos mas recientes.
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

    // 1) 从当前工作目录向上查找Buscar hacia arriba desde el directorio de trabajo actual.
    if (auto s = try_find(cwd); !s.empty()) return s;

    // 2) 从可执行文件所在目录向上查找 Busque hacia arriba desde el directorio donde se encuentra el archivo ejecutable.
    if (!baseStr.empty()) {
        if (auto s = try_find(fs::path(baseStr)); !s.empty()) return s;
    }

    std::cout << "[Path] Assets NOT found; fallback to literal 'Assets'." << std::endl;
    return "Assets";
}


// 加载 BakerHouse Cargar BakerHouse

static void loadBakerHouse() {
    namespace fs = std::filesystem;

    std::string assetsPath = getAssetsPath();
    fs::path bakerHousePath = fs::absolute(fs::path(assetsPath) / "BakerHouse.fbx");

    std::cout << "Attempting to load BakerHouse from: " << bakerHousePath.string() << std::endl;

    if (!fs::exists(bakerHousePath)) {
        std::cerr << "[ERROR] BakerHouse.fbx not found at: " << bakerHousePath.string() << std::endl;
        std::cerr << "-> Put the file here, OR drag & drop an FBX into the window." << std::endl;
        return;
    }

    auto meshes = ModelLoader::loadModel(bakerHousePath.string());
    if (meshes.empty()) {
        std::cerr << "[ERROR] Assimp failed to load model. Check console above for details." << std::endl;
        return;
    }

    std::cout << "BakerHouse loaded successfully with " << meshes.size() << " meshes." << std::endl;

    for (size_t i = 0; i < meshes.size(); i++) {
        auto gameObject = std::make_shared<GameObject>("BakerHouse_" + std::to_string(i));
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

    size_t lastSlash = filepath.find_last_of("/\\");
    size_t lastDot = filepath.find_last_of(".");
    string modelName = filepath.substr(lastSlash + 1, lastDot - lastSlash - 1);

    for (size_t i = 0; i < meshes.size(); i++) {
        auto gameObject = make_shared<GameObject>(modelName + "_" + to_string(i));
        gameObject->setMesh(meshes[i]);
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

// 绘制网格线与辅助三角 Dibujar líneas de cuadrícula y triángulos auxiliares
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
    camera.aspect = targetAspect;
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(&camera.projection()[0][0]);
}


// 输入与渲染循环 Bucle de entrada y renderizado
static void handle_input() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_QUIT:
            running = false;
            break;
        case SDL_EVENT_KEY_DOWN:
            if (event.key.scancode == SDL_SCANCODE_ESCAPE) running = false;
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
                if (selectedGameObject) selectedGameObject->isSelected = false;
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
        case SDL_EVENT_DROP_FILE:
            if (event.drop.data) {
                string droppedFile = event.drop.data;
                cout << "File dropped: " << droppedFile << endl;
                handleDropFile(droppedFile);
            }
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

    draw_floorGrid(16, 0.25);
    glColor3f(1.0f, 1.0f, 1.0f);
    for (const auto& go : gameObjects) go->draw();
    draw_triangle(Colors::Red, vec3(-1, 0.25, 0), 0.5);
    draw_triangle(Colors::Green, vec3(0, 0.5, 0.25), 0.5);
    draw_triangle(Colors::Blue, vec3(1, -0.5, -0.25), 0.5);
    editor.render();
    SDL_GL_SwapWindow(window);
}


// OpenGL 初始化 Inicializacion de OpenGL

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


// 程序入口 punto de entrada del programa

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
    window = SDL_CreateWindow("Motor - FBX Loader", screenWidth, screenHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        cout << "Window could not be created! " << SDL_GetError() << endl;
        return EXIT_FAILURE;
    }

    glContext = SDL_GL_CreateContext(window);
    editor.init(window, glContext);
    editor.setScene(&gameObjects, &selectedGameObject);
    if (!glContext) {
        cout << "OpenGL context could not be created!" << endl;
        return EXIT_FAILURE;
    }

    init_opengl();
    camera.transform().pos() = vec3(0, 5, 10);
    camera.orbitTarget = vec3(0, 0, 0);
    updateProjection(screenWidth, screenHeight);

    cout << "========================================" << endl;
    cout << "Loading BakerHouse..." << endl;
    cout << "========================================" << endl;
    loadBakerHouse();

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
        SDL_Delay(1);
    }

    gameObjects.clear();
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    editor.shutdown();
    SDL_Quit();
    return EXIT_SUCCESS;
}
