#include <iostream>
#include <cstdlib>

// ⚠️ GLEW 必须在任何 gl.h 之前
#include <GL/glew.h>

// SDL3 main 支持
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

// 如果 Camera.cpp / 其他文件用了 extern SDL_Window*
SDL_Window* window = nullptr;

static void PrintSdlDiagnostics()
{
    std::cout << "SDL revision: '" << SDL_GetRevision() << "'\n";
    std::cout << "SDL platform: '" << SDL_GetPlatform() << "'\n";

    const char* env_driver = std::getenv("SDL_VIDEODRIVER");
    std::cout << "Env SDL_VIDEODRIVER: '" << (env_driver ? env_driver : "") << "'\n";

    int n = SDL_GetNumVideoDrivers();
    std::cout << "SDL_GetNumVideoDrivers() = " << n << "\n";
    for (int i = 0; i < n; ++i)
    {
        std::cout << "  driver[" << i << "] = '" << SDL_GetVideoDriver(i) << "'\n";
    }

    std::cout << "SDL_GetError() = '" << SDL_GetError() << "'\n";
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    PrintSdlDiagnostics();

    // ✅ SDL3：如果你自己写 main，必须调用
    SDL_SetMainReady();

    std::cout << "Initializing SDL core...\n";

    // ⚠️ SDL3 返回 bool：true = success, false = failure
    if (!SDL_Init(0)) {
        std::cerr << "SDL_Init(0) failed: '" << SDL_GetError() << "'\n";
        return -1;
    }

    if (!SDL_InitSubSystem(SDL_INIT_EVENTS)) {
        std::cerr << "SDL_InitSubSystem(EVENTS) failed: '" << SDL_GetError() << "'\n";
        SDL_Quit();
        return -2;
    }

    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_InitSubSystem(VIDEO) failed: '" << SDL_GetError() << "'\n";
        SDL_Quit();
        return -3;
    }

    std::cout << "SDL initialized successfully.\n";

    // ----------------------------
    // OpenGL attributes
    // ----------------------------
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // ----------------------------
    // Window
    // ----------------------------
    window = SDL_CreateWindow(
        "MotorA2",
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        std::cerr << "SDL_CreateWindow failed: '" << SDL_GetError() << "'\n";
        SDL_Quit();
        return -4;
    }

    // ----------------------------
    // OpenGL context
    // ----------------------------
    SDL_GLContext glctx = SDL_GL_CreateContext(window);
    if (!glctx) {
        std::cerr << "SDL_GL_CreateContext failed: '" << SDL_GetError() << "'\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -5;
    }

    SDL_GL_MakeCurrent(window, glctx);
    SDL_GL_SetSwapInterval(1); // VSync

    // ----------------------------
    // GLEW
    // ----------------------------
    glewExperimental = GL_TRUE;
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        std::cerr << "glewInit failed: "
            << reinterpret_cast<const char*>(glewGetErrorString(glewErr)) << "\n";
        SDL_GL_DestroyContext(glctx);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -6;
    }

    // 清掉 GLEW 的一个已知 GL 错误
    glGetError();

    std::cout << "OpenGL Vendor  : " << glGetString(GL_VENDOR) << "\n";
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << "\n";
    std::cout << "OpenGL Version : " << glGetString(GL_VERSION) << "\n";

    // ----------------------------
    // Main loop
    // ----------------------------
    bool running = true;
    while (running)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT)
                running = false;
        }

        int w = 1280, h = 720;
        SDL_GetWindowSize(window, &w, &h);

        glViewport(0, 0, w, h);
        glEnable(GL_DEPTH_TEST);

        glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        SDL_GL_SwapWindow(window);
    }

    // ----------------------------
    // Cleanup
    // ----------------------------
    SDL_GL_DestroyContext(glctx);
    SDL_DestroyWindow(window);
    window = nullptr;

    SDL_Quit();
    return 0;
}
