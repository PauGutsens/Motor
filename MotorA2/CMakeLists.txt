cmake_minimum_required(VERSION 3.23)

project(MotorA2 LANGUAGES CXX)

# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 使用 vcpkg 的 manifest 模式（读取 vcpkg.json）
set(VCPKG_MANIFEST_MODE ON)

# ---------------------------------------------------------
# 源码：递归收集 Source 下面所有 .cpp / .h
# 目录结构假设为：
#   Source/
#     Core/
#     Renderer/
#     Editor/
#     main.cpp
# ---------------------------------------------------------
file(GLOB_RECURSE MOTOR_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
)
file(GLOB_RECURSE MOTOR_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
)

# Exclude TextureStubs.cpp now that real TextureLoader is enabled
# 现在启用了真正的 TextureLoader，这里把临时的 TextureStubs.cpp 排除
list(FILTER MOTOR_SOURCES EXCLUDE REGEX "TextureStubs.cpp$")

add_executable(MotorA2
    ${MOTOR_SOURCES}
    ${MOTOR_HEADERS}
)

# ---------------------------------------------------------
# 通过 vcpkg 提供的第三方库
# ---------------------------------------------------------
find_package(SDL3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

target_link_libraries(MotorA2 PRIVATE
    SDL3::SDL3
    OpenGL::GL
    GLEW::GLEW
    assimp::assimp
    glm::glm
    imgui::imgui
)

# 让编译器能找到各个模块的头文件
target_include_directories(MotorA2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Renderer
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Editor
)
