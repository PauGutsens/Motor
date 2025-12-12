#pragma once
#include <GL/glew.h>
#include <iostream>

class Framebuffer {
public:
    Framebuffer();
    ~Framebuffer();

    void Init(int width, int height);
    void Rescale(int width, int height);
    void Bind();
    void Unbind();

    GLuint GetTextureID() const { return textureID; }
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

private:
    GLuint fboID = 0;
    GLuint textureID = 0;
    GLuint rboID = 0;
    int width = 0;
    int height = 0;
};
