#include <functional>
#include <imgui.h>

// main.cpp
// 中英双语注释 / Chinese-English bilingual comments
//
// 目标 / Goal
// 1) 用 SDL3 创建窗口 + OpenGL 上下文 / Create SDL3 window + OpenGL context
// 2) 初始化 GLEW 以使用 OpenGL 函数 / Init GLEW for OpenGL function loading
// 3) 启动 ImGui + 编辑器窗口系统 / Start ImGui + editor windows
// 4) 进入主循环：事件 -> UI/场景渲染 / Main loop: events -> UI/scene render
#define SDL_MAIN_HANDLED


#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>   // ✅ 加这一行，保证 SDL_SetMainReady 声明可见
#include <GL/glew.h> // OpenGL function loader / OpenGL 函数加载器

#include <iostream>
#include <vector>
#include <memory>
#include <glm/gtc/type_ptr.hpp>   // 为 glm::value_ptr 提供声明

// Project headers / 项目头文件
#include "Editor/EditorWindows.h"
#include "Editor/Camera.h"
#include "Core/GameObject.h"

// 让 Camera.cpp 能访问窗口（它用了 extern SDL_Window* window）
// Let Camera.cpp access the window (it uses extern SDL_Window* window)
SDL_Window* window = nullptr;

// 小工具：打印运行信息（可选）
// Helper: print runtime info (optional)
//static void PrintRuntimeInfo()
//{
//    // Chinese: SDL3 用 SDL_GetVersion 输出版本号
//    // English: SDL3 uses SDL_GetVersion for version numbers
//    int major = 0, minor = 0, patch = 0;
//    SDL_GetVersion(&major, &minor, &patch);
//    std::cout << "SDL version: " << major << "." << minor << "." << patch << "\n";
//}
static void PrintRuntimeInfo()
{
    //int major = 0, minor = 0, patch = 0;

    //// SDL3: SDL_GetVersion() returns SDL_Version
    //SDL_Version v = SDL_GetVersion();
    //major = (int)v.major;
    //minor = (int)v.minor;
    //patch = (int)v.patch;

    //std::cout << "SDL version: " << major << "." << minor << "." << patch << "\n";
}

int main(int argc, char** argv)
{
    (void)argc; (void)argv;

    PrintRuntimeInfo();
    SDL_SetMainReady();
    // 1) Init SDL / 初始化 SDL
    //if (SDL_Init(SDL_INIT_VIDEO) != 0)
    //{
    //    std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
    //    return 1;
    //}
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    // 2) Request OpenGL 3.3 Core / 请求 OpenGL 3.3 Core
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // 3) Create window / 创建窗口
    window = SDL_CreateWindow("MotorA2", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    // 4) Create GL context / 创建 OpenGL 上下文
    SDL_GLContext glctx = SDL_GL_CreateContext(window);
    if (!glctx)
    {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // 5) Make context current + VSync / 设置当前上下文 + 垂直同步
    SDL_GL_MakeCurrent(window, glctx);
    SDL_GL_SetSwapInterval(1);

    // 6) Init GLEW / 初始化 GLEW
    // Chinese: 必须在创建 GL context 之后调用，否则会失败
    // English: must be called AFTER GL context is created
    glewExperimental = GL_TRUE;
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK)
    {
        std::cerr << "glewInit failed: " << (const char*)glewGetErrorString(glewErr) << "\n";
        SDL_GL_DestroyContext(glctx);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // 打印 OpenGL 信息 / Print OpenGL info
    std::cout << "OpenGL Vendor  : " << glGetString(GL_VENDOR) << "\n";
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << "\n";
    std::cout << "OpenGL Version : " << glGetString(GL_VERSION) << "\n";

    // 7) Create scene + camera / 创建场景 + 相机
    std::vector<std::shared_ptr<GameObject>> scene;
    std::shared_ptr<GameObject> selected = nullptr;

    Camera mainCam;
    // Chinese: 初始 aspect 用窗口比例；后面窗口 resize 时也会更新
    // English: set initial aspect; update again on resize
    mainCam.aspect = 1280.0f / 720.0f;
    mainCam.transform.setPosition(vec3(0.0, 1.5, 8.0));  // 先拉远一点
    mainCam.focusOn(vec3(0.0), 5.0);                     // 给一个默认 orbit 距离


    // 8) Init EditorWindows / 初始化编辑器窗口系统
    //EditorWindows editor;
    //editor.setScene(&scene, selected);
    //editor.setMainCamera(&mainCam);
    //editor.init(window, glctx);

    //// 9) Main loop / 主循环
    //bool running = true;
    ////while (running && !editor.wantsQuit())
    ////{
    ////    SDL_Event e;
    ////    while (SDL_PollEvent(&e))
    ////    {
    ////        // 先喂给编辑器（里面会喂给 ImGui） / Feed editor first (it forwards to ImGui)
    ////        editor.processEvent(e);

    ////        // 如果是窗口关闭事件 / If window close event
    ////        if (e.type == SDL_EVENT_QUIT)
    ////            running = false;

    ////        // 窗口尺寸变化：更新 viewport + 相机 aspect
    ////        // Window resized: update viewport + camera aspect
    ////        if (e.type == SDL_EVENT_WINDOW_RESIZED)
    ////        {
    ////            int w = 0, h = 0;
    ////            SDL_GetWindowSize(window, &w, &h);
    ////            if (h > 0)
    ////                mainCam.aspect = float(w) / float(h);

    ////            glViewport(0, 0, w, h);
    ////        }
    ////    }


    ////    // 清屏 / Clear
    ////    glEnable(GL_DEPTH_TEST);
    ////    glClearColor(0.12f, 0.12f, 0.13f, 1.0f);
    ////    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    ////    // 渲染编辑器 UI + 场景 / Render editor UI + scene
    ////    editor.render(mainCam);

    ////    // Swap / 交换缓冲
    ////    SDL_GL_SwapWindow(window);
    ////}
    //while (running && !editor.wantsQuit())
    EditorWindows editor;
    editor.setScene(&scene);
    editor.init(window, glctx);
    uint64_t prevCounter = SDL_GetPerformanceCounter();
    double perfFreq = (double)SDL_GetPerformanceFrequency();

    bool running = true;
    while (running)
    

    {
        uint64_t nowCounter = SDL_GetPerformanceCounter();
        double dt = (double)(nowCounter - prevCounter) / perfFreq;
        prevCounter = nowCounter;

        mainCam.update(dt);

        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            editor.processEvent(e);
            ImGuiIO& io = ImGui::GetIO();

            switch (e.type)
            {
            case SDL_EVENT_QUIT:
                running = false;
                break;

            case SDL_EVENT_WINDOW_RESIZED:
            {
                int w = 0, h = 0;
                SDL_GetWindowSizeInPixels(window, &w, &h);
                if (h > 0) mainCam.aspect = (double)w / (double)h;
                glViewport(0, 0, w, h);
                break;
            }

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (!io.WantCaptureMouse)
                    mainCam.onMouseButton(e.button.button, 1, (int)e.button.x, (int)e.button.y);
                break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (!io.WantCaptureMouse)
                    mainCam.onMouseButton(e.button.button, 0, (int)e.button.x, (int)e.button.y);
                break;

            case SDL_EVENT_MOUSE_MOTION:
                if (!io.WantCaptureMouse)
                    mainCam.onMouseMove((int)e.motion.x, (int)e.motion.y);
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                if (!io.WantCaptureMouse)
                    mainCam.onMouseWheel((int)e.wheel.y); // y: +上滚 / -下滚
                break;

            case SDL_EVENT_KEY_DOWN:
                if (!io.WantCaptureKeyboard)
                    mainCam.onKeyDown(e.key.scancode);
                break;

            case SDL_EVENT_KEY_UP:
                if (!io.WantCaptureKeyboard)
                    mainCam.onKeyUp(e.key.scancode);
                break;

            default:
                break;
            }

            if (e.type == SDL_EVENT_QUIT)
                running = false;
        }

        //// ✅ 1) 先开始 ImGui 新的一帧（必须每帧一次）
        //editor.newFrame();
        //int w = 0, h = 0;
        //SDL_GetWindowSizeInPixels(window, &w, &h);   // SDL3 正确拿像素尺寸
        //// ✅ 2) 渲染你的世界（可选：先画3D再画UI，或反过来都行）
        //glViewport(0, 0, w, h);
        //glEnable(GL_DEPTH_TEST);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //// draw scene...
        //// for(auto& go : scene) go->draw(camera);  // 你的项目具体怎么画按你现在的写法

        //// ✅ 3) 最后渲染 UI（里面会 ImGui::Render）
        //editor.render(&mainCam);

        //// ✅ 4) swap
        //SDL_GL_SwapWindow(window);
        // ✅ 1) 每帧开始 ImGui
        editor.newFrame();

        // ✅ 2) 拿到窗口像素尺寸 -> viewport
        int w = 0, h = 0;
        SDL_GetWindowSizeInPixels(window, &w, &h);
        glViewport(0, 0, w, h);

        // ✅ 3) 清屏 + 深度
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_LIGHTING);     // 关键：避免没有灯光导致全黑
        glDisable(GL_TEXTURE_2D);   // 你现在 TextureID=0，先明确关掉
        glColor3f(0.8f, 0.8f, 0.8f);// 给一个默认颜色，让面可见
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glLineWidth(1.0f);
        glColor3f(0.8f, 0.8f, 0.8f);


        // ✅ 4) 固定管线：把 Camera 的投影/视图矩阵加载进 OpenGL
        // （因为你的 GameObject::draw() 用的是 glPushMatrix/glMultMatrixd，必须走 fixed pipeline）
        {
            mainCam.aspect = (h > 0) ? (double)w / (double)h : mainCam.aspect;

            mat4 P = mainCam.projection();
            mat4 V = mainCam.view();

            glMatrixMode(GL_PROJECTION);
            glLoadMatrixd(glm::value_ptr(P));

            glMatrixMode(GL_MODELVIEW);
            glLoadMatrixd(glm::value_ptr(V));
        }

        // ✅ 5) 画场景（递归画 root + children）
        if (!scene.empty())
        {
            std::function<void(GameObject*)> drawNode = [&](GameObject* n)
                {
                    if (!n) return;
                    n->draw();
                    for (auto* c : n->children) drawNode(c);
                };

            for (auto& sp : scene)
            {
                if (!sp) continue;
                if (sp->parent != nullptr) continue;   // 只从 root 开始递归
                drawNode(sp.get());
            }
        }

        // ✅ 6) 最后画 UI
        editor.render(&mainCam);

        // ✅ 7) swap
        SDL_GL_SwapWindow(window);

    }


    // 10) Shutdown / 关闭
    editor.shutdown();
    SDL_GL_DestroyContext(glctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
