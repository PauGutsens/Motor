#include "TextureLoader.h"
#include "AssetMeta.h"
#include <GL/glew.h>
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
    AssetMeta meta;
    std::string metaPath = path + ".meta";
    bool hasMeta = AssetMeta::loadFromFile(metaPath, meta);
    ILuint img = 0;
    ilGenImages(1, &img);
    ilBindImage(img);
    if (!ilLoadImage(path.c_str())) {
        ilDeleteImages(1, &img);
        return 0;
    }
    if (hasMeta) {
        if (meta.texFlipY) iluFlipImage();
        if (meta.texFlipX) iluMirror();
    }
    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
    const int w = ilGetInteger(IL_IMAGE_WIDTH);
    const int h = ilGetInteger(IL_IMAGE_HEIGHT);
    const void* pixels = ilGetData();
    if (!pixels || w <= 0 || h <= 0) {
        ilDeleteImages(1, &img);
        return 0;
    }
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    GLint minf = GL_LINEAR, magf = GL_LINEAR, wrapS = GL_REPEAT, wrapT = GL_REPEAT;
    if (hasMeta) {
        if (meta.texMinFilter == "Nearest") minf = GL_NEAREST;
        else if (meta.texMinFilter == "Trilinear") minf = GL_LINEAR_MIPMAP_LINEAR;
        if (meta.texMagFilter == "Nearest") magf = GL_NEAREST;
        if (meta.texWrapS == "ClampToEdge") wrapS = GL_CLAMP_TO_EDGE;
        else if (meta.texWrapS == "MirroredRepeat") wrapS = GL_MIRRORED_REPEAT;
        if (meta.texWrapT == "ClampToEdge") wrapT = GL_CLAMP_TO_EDGE;
        else if (meta.texWrapT == "MirroredRepeat") wrapT = GL_MIRRORED_REPEAT;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minf);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magf);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
    if (hasMeta && (meta.texMipmaps || meta.texMinFilter == "Trilinear")) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    ilBindImage(0);
    ilDeleteImages(1, &img);
    return tex;
}
