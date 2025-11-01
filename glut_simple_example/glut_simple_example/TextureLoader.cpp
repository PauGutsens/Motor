#include "TextureLoader.h"
#include <IL/il.h>
#include <IL/ilu.h>
#include <SDL3/SDL_opengl.h>  

static bool g_devILInited = false;

void EnsureDevILInited() {
    if (!g_devILInited) {
        ilInit();
        iluInit();
        g_devILInited = true;
    }
}

unsigned int LoadTexture2D(const std::string& path) {
    EnsureDevILInited();

    ILuint img = 0;
    ilGenImages(1, &img);
    ilBindImage(img);

    if (!ilLoadImage(path.c_str())) {
        ilDeleteImages(1, &img);
        return 0;
    }

    // תΪ RGBA8 Convertir a RGBA8
    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

    const int w = ilGetInteger(IL_IMAGE_WIDTH);
    const int h = ilGetInteger(IL_IMAGE_HEIGHT);
    const void* pixels = ilGetData();
    if (!pixels || w <= 0 || h <= 0) {
        ilDeleteImages(1, &img);
        return 0;
    }

    // ���� OpenGL ���� Generar texturas OpenGL
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // ������ glGenerateMipmap����Ҫ��չ��������No llame a glGenerateMipmap (requiere un cargador de extensiones).

    ilBindImage(0);
    ilDeleteImages(1, &img);
    return tex; // 0 ��ʾʧ�� 0 indica fallo
}
