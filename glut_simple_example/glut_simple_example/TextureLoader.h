#pragma once
#include <string>
#include <cstdint>

// 成功返回 OpenGL 纹理ID(>0)，失败返回 0
unsigned int LoadTexture2D(const std::string& path);

// 可在程序启动处调用一次；如果你已经在别处 ilInit() 过，可以不调用
void EnsureDevILInited();
