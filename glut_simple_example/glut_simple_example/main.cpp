#include <iostream>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include "Camera.h"
#include <chrono>
using namespace std;

static Camera camera;
static auto lastFrameTime = chrono::high_resolution_clock::now();
SDL_Window* window = nullptr;
static SDL_GLContext glContext = nullptr;
static bool running = true;

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
	glColor3ub(0, 0, 0);
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
	// Keep a 16:9 aspect
    double targetAspect = 16.0 / 9.0;
    int viewportWidth = width;
    int viewportHeight = height;
    int viewportX = 0;
    int viewportY = 0;

    // Calculate the viewport size
    double currentAspect = static_cast<double>(width) / height;
    
    if (currentAspect > targetAspect) {
        viewportWidth = static_cast<int>(height * targetAspect);
        viewportX = (width - viewportWidth) / 2;
    }
    else {
        viewportHeight = static_cast<int>(width / targetAspect);
        viewportY = (height - viewportHeight) / 2;
    }

    // Set up the viewport and update
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
                if (event.key.scancode == SDL_SCANCODE_ESCAPE) {running = false;} 
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

    glClearColor(0.5, 0.5, 0.5, 1.0);
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
    window = SDL_CreateWindow("Motor", screenWidth, screenHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

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
    camera.transform().pos() = vec3(0, 1, 4);
    camera.orbitTarget = vec3(0, 0.25, 0);

    updateProjection(screenWidth, screenHeight);

    // Main game loop
    while (running) {
        handle_input();
        render();
        SDL_Delay(1);
    }

    // Clean up
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}