cmake_minimum_required(VERSION 3.10)

project(Engine)

find_package(SDL3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(DevIl REQUIRED)

add_executable(Motor src/helloworld.cpp )

target_link_libraries(Motor PRIVATE SDL3::SDL3)
target_link_libraries(Motor PRIVATE glm::glm)
target_link_libraries(Motor PRIVATE assimp::assimp)
target_link_libraries(Motor PRIVATE DevIL::IL)