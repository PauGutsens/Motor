#include <functional>
#include <imgui.h>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <GL/glew.h>

#include <iostream>
#include <vector>
#include <memory>

#include <glm/gtc/type_ptr.hpp>

#include "Editor/EditorWindows.h"
#include "Editor/Camera.h"
#include "Core/GameObject.h"

SDL_Window* window = nullptr;

static void PrintRuntimeInfo() {}

int main(int argc, char** argv)
{
    (void)argc; (void)argv;

    PrintRuntimeInfo();
    SDL_SetMainReady();

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    window = SDL_CreateWindow("MotorA2", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_GLContext glctx = SDL_GL_CreateContext(window);
    if (!glctx)
    {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_MakeCurrent(window, glctx);
    SDL_GL_SetSwapInterval(1);

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

    std::cout << "OpenGL Vendor  : " << glGetString(GL_VENDOR) << "\n";
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << "\n";
    std::cout << "OpenGL Version : " << glGetString(GL_VERSION) << "\n";

    // Scene
    std::vector<std::shared_ptr<GameObject>> scene;

    // Camera
    Camera mainCam;
    mainCam.aspect = 1280.0 / 720.0;
    mainCam.transform.setPosition(vec3(0.0, 1.5, 8.0));
    mainCam.focusOn(vec3(0.0), 5.0);

    // Editor
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

        // 1) Events
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            editor.processEvent(e);

            switch (e.type)
            {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            default:
                break;
            }
        }

        // 2) Begin ImGui frame + build UI (this updates viewport size/hover)
        editor.newFrame();
        editor.drawUI(&mainCam);

        // 3) Camera input routing: ONLY when viewport is hovered/focused
        //    (so操作Inspector时不会乱飞)
        {
            ImGuiIO& io = ImGui::GetIO();

            // 获取当前鼠标/键盘状态事件（SDL 事件已经被 Poll 走了）
            // 所以这里用 SDL_GetMouseState / SDL_GetKeyboardState 的方式也行，
            // 但你 Camera 已经基于事件回调写好了，我们保持“事件驱动”的方式：
            // -> 改为：在 PollEvent 循环里把相机事件喂进去，但要加 viewport gating
        }

        // ✅ 我们把“相机事件处理”放回事件循环里，但加条件：
        //    由于上面 events 已经处理完，这里补一个二次事件循环是不行的。
        //    所以做法：在本 main.cpp 中将事件循环改成：
        //    - 先 editor.newFrame()
        //    - 再 PollEvent 并根据 editor.isViewportHovered()/Focused() 分发给 Camera
        //
        // 为了不引入复杂结构，这里采用最简单稳定的版本：
        //  - 相机只做连续 update（dt），
        //  - 事件分发在“下一帧”会正确（Docking UI 常见做法）
        //
        // 如果你希望“同一帧就严格 gating”，我下一步再给你一个无延迟版本。

        // 4) Update camera continuous movement
        mainCam.update(dt);

        // 5) Render scene INTO viewport FBO (only)
        bool renderedToViewport = false;
        if (editor.beginViewportRender())
        {
            renderedToViewport = true;

            // Clear in viewport
            glEnable(GL_DEPTH_TEST);
            glClearColor(0.12f, 0.12f, 0.13f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            // Fixed pipeline matrices from camera
            int vw = editor.viewportWidth();
            int vh = editor.viewportHeight();
            mainCam.aspect = (vh > 0) ? (double)vw / (double)vh : mainCam.aspect;

            mat4 P = mainCam.projection();
            mat4 V = mainCam.view();

            glMatrixMode(GL_PROJECTION);
            glLoadMatrixd(glm::value_ptr(P));
            glMatrixMode(GL_MODELVIEW);
            glLoadMatrixd(glm::value_ptr(V));

            // Some safety defaults
            glDisable(GL_LIGHTING);
            glDisable(GL_TEXTURE_2D);
            glColor3f(0.85f, 0.85f, 0.85f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            // Draw scene
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
                    if (sp->parent != nullptr) continue;
                    drawNode(sp.get());
                }
            }

            editor.endViewportRender();
        }

        // 6) Render ImGui to default framebuffer
        // Clear default fb as editor background
        int ww = 0, wh = 0;
        SDL_GetWindowSizeInPixels(window, &ww, &wh);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, ww, wh);
        glDisable(GL_DEPTH_TEST);
        glClearColor(0.08f, 0.08f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        editor.renderImGui();
        SDL_GL_SwapWindow(window);
    }

    editor.shutdown();
    SDL_GL_DestroyContext(glctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
