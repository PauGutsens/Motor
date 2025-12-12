#include "TextureLoader.h"
#include <string>

// 这是一个“空初始化”，现在先不真正初始化 DevIL
void EnsureDevILInited() {
    // 以后如果要接 DevIL 或 stb_image，可以在这里做初始化
}

// 简易的纹理加载 stub：目前只返回 0，表示“没有真正加载纹理”
unsigned int LoadTexture2D(const std::string& path) {
    // TODO: 后面可以换成 stb_image 或 DevIL 的真实实现
    return 0;
}
