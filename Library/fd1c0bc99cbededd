cmake_minimum_required(VERSION 3.10)

project(Motor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(DevIL REQUIRED)
find_package(imgui CONFIG REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")
file(GLOB_RECURSE HEADERS "src/*.h" "src/*.hpp")

add_executable(Motor ${SOURCES} ${HEADERS})

target_include_directories(Motor PRIVATE "src")

target_link_libraries(Motor PRIVATE 
    fmt::fmt
    SDL3::SDL3
    glm::glm
    assimp::assimp
    GLEW::GLEW
    DevIL::IL DevIL::ILU
    imgui::imgui 
)