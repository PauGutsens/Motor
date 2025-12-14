#include "TextureLoader.h"

#include <GL/glew.h>
#include <SDL3/SDL_opengl.h>
#include <string>
#include <iostream>

// We use stb_image as a lightweight image loader.
// 使用 stb_image 作为轻量级图片加载库。
// 请把官方的 "stb_image.h" 放到 Source/Renderer 目录下。
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static bool g_stbInited = false;

void EnsureDevILInited()
{
    // For stb_image we only need to configure it once.
    // 对于 stb_image，我们只需要做一次全局配置。
    if (!g_stbInited)
    {
        // Most DCC tools export textures with origin at top-left;
        // OpenGL expects bottom-left. Flip vertically on load.
        // 大多数 DCC 导出纹理的原点在左上，而 OpenGL 以左下为原点，这里做一次垂直翻转。
        stbi_set_flip_vertically_on_load(true);
        g_stbInited = true;
    }
}

unsigned int LoadTexture2D(const std::string& path)
{
    EnsureDevILInited();

    int w = 0, h = 0, channels = 0;

    // Force 4 channels (RGBA) for simplicity.
    // 为简单起见，强制转换为 RGBA 四通道。
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, STBI_rgb_alpha);
    if (!data)
    {
        std::cerr << "Failed to load texture: " << path
            << " reason: " << stbi_failure_reason() << std::endl;
        return 0;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        w,
        h,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        data);

    // Basic sampler state / 基本采样状态
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    return tex;
}
