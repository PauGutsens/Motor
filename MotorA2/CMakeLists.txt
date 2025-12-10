cmake_minimum_required(VERSION 3.23)
project(MotorA2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(VCPKG_MANIFEST_MODE ON)

file(GLOB MOTOR_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
)
file(GLOB MOTOR_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
)

add_executable(MotorA2
    ${MOTOR_SOURCES}
    ${MOTOR_HEADERS}
)

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

target_include_directories(MotorA2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
)
