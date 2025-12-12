#include <GL/glew.h>            // ⭐ 必须第一个
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <iostream>

#include "Camera.h"
#include "EditorWindows.h"
#include "Logger.h"




// ⭐ 全局窗口指针（Camera.cpp 需要）
// （extern SDL_Window* window 在 Camera.cpp 里声明过）
SDL_Window* window = nullptr;

// 选择 OpenGL 版本（ImGui 后端也依赖 3.0+）
static const char* glsl_version = "#version 150";

int main(int argc, char** argv)
{
    // ---------------------------------------------------
    // 初始化 SDL
    // ---------------------------------------------------
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    // 设置 OpenGL 环境（Core Profile、3.2+）
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // ---------------------------------------------------
    // 创建窗口（全局 window 要在这里赋值）
    // ---------------------------------------------------
    window = SDL_CreateWindow(
        "MotorA2 Engine",
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    // ---------------------------------------------------
    // 创建 OpenGL 上下文
    // ---------------------------------------------------
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // 开启 VSync

    std::cout << "MotorA2 started successfully.\n";

    // ---------------------------------------------------
    // 初始化你引擎的系统（可根据你们原来结构继续扩展）
    // ---------------------------------------------------
    Camera camera;
    EditorWindows editor;

    bool running = true;

    // ---------------------------------------------------
    // 主循环
    // ---------------------------------------------------
    while (running) {

        // -------------------
        // 处理事件
        // -------------------
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }
            // 可加入 Editor / Camera 控制
        }

        // -------------------
        // 清屏
        // -------------------
        glViewport(0, 0, 1280, 720);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // -------------------
        // TODO: 这里渲染 3D 场景
        // - 使用 Camera
        // - 使用 Mesh / ModelLoader
        // - 使用 Renderer 系统
        // -------------------

        // -------------------
        // TODO: 渲染 ImGui 编辑器界面
        // editor.Draw();
        // -------------------

        // -------------------
        // 显示这一帧
        // -------------------
        SDL_GL_SwapWindow(window);
    }

    // ---------------------------------------------------
    // 清理
    // ---------------------------------------------------
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
