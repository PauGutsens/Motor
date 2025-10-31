cmake_minimum_required(VERSION 3.10)

project(Motor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(SDL3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(GLEW REQUIRED)

# DevIL - RUTAS ABSOLUTAS FORZADAS
set(IL_INCLUDE_DIR "C:/vcpkg/installed/x64-windows/include")
set(IL_LIBRARY "C:/vcpkg/installed/x64-windows/lib/DevIL.lib")
set(ILU_LIBRARY "C:/vcpkg/installed/x64-windows/lib/ILU.lib")

# Verificar que existen
if(NOT EXISTS "${IL_INCLUDE_DIR}/IL/il.h")
    message(FATAL_ERROR "DevIL headers not found at: ${IL_INCLUDE_DIR}/IL/il.h")
endif()

if(NOT EXISTS "${IL_LIBRARY}")
    message(FATAL_ERROR "DevIL library not found at: ${IL_LIBRARY}")
endif()

message(STATUS "===================================")
message(STATUS "✓ DevIL Include: ${IL_INCLUDE_DIR}")
message(STATUS "✓ DevIL Library: ${IL_LIBRARY}")
message(STATUS "✓ ILU Library: ${ILU_LIBRARY}")
message(STATUS "===================================")

# Source files
set(SOURCES
    src/main.cpp
    src/Camera.cpp
    src/Transform.cpp
    src/Mesh.cpp
    src/GameObject.cpp
    src/ModelLoader.cpp
)

# Header files
set(HEADERS
    src/Camera.h
    src/Transform.h
    src/types.h
    src/Mesh.h
    src/GameObject.h
    src/ModelLoader.h
)

# Create executable
add_executable(Motor ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(Motor PRIVATE 
    SDL3::SDL3
    glm::glm
    assimp::assimp
    GLEW::GLEW
    ${IL_LIBRARY}
    ${ILU_LIBRARY}
)

# Include directories - IMPORTANTE: Agrega el include ANTES
target_include_directories(Motor PRIVATE 
    ${IL_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Copy Assets folder to build directory
add_custom_command(TARGET Motor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/../Assets
    $<TARGET_FILE_DIR:Motor>/Assets
    COMMENT "Copying Assets folder to build directory"
)

if(WIN32)
    add_custom_command(TARGET Motor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Remember to copy DevIL.dll and ILU.dll to executable directory"
    )
endif()