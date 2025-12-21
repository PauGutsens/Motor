#include <functional>
#include <imgui.h>
#include "Core/Frustum.h"

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

int main(int argc, char** argv)
{
    (void)argc; (void)argv;

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
    //mainCam.transform.setPosition(vec3(0.0, 1.5, 8.0));
    //mainCam.focusOn(vec3(0.0), 5.0);
    mainCam.transform.setPosition(vec3(0.0, 1.5, -8.0));
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

        // 1) Begin ImGui frame first (so io exists)
        editor.newFrame();

        // 2) Events (camera input gating: ONLY if viewport hovered/focused OR relative mouse mode)
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            editor.processEvent(e);

            if (e.type == SDL_EVENT_QUIT)
            {
                running = false;
                break;
            }

            // 相机是否允许吃输入：
            // - 鼠标在 Viewport 上，允许
            // - 或者已经进入 relative mouse mode（RMB 按住时），继续允许
            const bool camMouseAllowed =
                editor.isViewportHovered() || editor.isViewportFocused() ||
                SDL_GetWindowRelativeMouseMode(window);

            const bool camKeyAllowed =
                editor.isViewportFocused() || SDL_GetWindowRelativeMouseMode(window);

            switch (e.type)
            {
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (camMouseAllowed)
                    mainCam.onMouseButton(e.button.button, 1, (int)e.button.x, (int)e.button.y);
                break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
                // 注意：释放一定要传给相机，避免 relative mode 卡住
                mainCam.onMouseButton(e.button.button, 0, (int)e.button.x, (int)e.button.y);
                break;

            case SDL_EVENT_MOUSE_MOTION:
                if (camMouseAllowed)
                    mainCam.onMouseMove((int)e.motion.x, (int)e.motion.y);
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                if (camMouseAllowed)
                    mainCam.onMouseWheel((int)e.wheel.y);
                break;

            case SDL_EVENT_KEY_DOWN:
                if (camKeyAllowed)
                    mainCam.onKeyDown(e.key.scancode);
                break;

            case SDL_EVENT_KEY_UP:
                if (camKeyAllowed)
                    mainCam.onKeyUp(e.key.scancode);
                break;

            default:
                break;
            }
        }

        // 3) Update camera
        mainCam.update(dt);

        // 4) Build UI (this updates viewport size W/H)
        editor.drawUI(&mainCam);

        // 5) Render scene into Viewport FBO
        if (editor.beginViewportRender())
        {
            // viewport size -> camera aspect
            int vw = editor.viewportWidth();
            int vh = editor.viewportHeight();
            if (vh > 0) mainCam.aspect = (double)vw / (double)vh;

            glEnable(GL_DEPTH_TEST);
            glClearColor(0.75f, 0.75f, 0.75f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            // fixed pipeline matrices
            mat4 P = mainCam.projection();
            mat4 V = mainCam.view();
            mat4 VP = P * V;
            Frustum fr = Frustum::fromViewProjection(VP);

            int drawn = 0;
            int culled = 0;
            glMatrixMode(GL_PROJECTION);
            glLoadMatrixd(glm::value_ptr(P));
            glMatrixMode(GL_MODELVIEW);
            glLoadMatrixd(glm::value_ptr(V));

            // safety defaults (避免黑屏)
            glDisable(GL_LIGHTING);
            glDisable(GL_TEXTURE_2D);
            glColor3f(0.85f, 0.85f, 0.85f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            /*if (!scene.empty())
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
            }*/
            if (!scene.empty())
            {
                std::function<void(GameObject*)> drawNode = [&](GameObject* n)
                    {
                        if (!n) return;

                        // Cull whole subtree using aggregated world AABB
                        AABB box = n->computeWorldAABB();
                        if (!fr.intersects(box))
                        {
                            culled++;
                            return;
                        }

                        // Draw this node (if it has mesh), then children
                        n->draw();
                        if (n->mesh) drawn++;

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
