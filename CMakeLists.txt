cmake_minimum_required(VERSION 3.10)

project(Motor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
    add_compile_options(/utf-8)
endif()

# --------------------------------------------------------
# BUSCANDO PAQUETES
# --------------------------------------------------------
find_package(SDL3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(DevIL REQUIRED)
find_package(fmt CONFIG REQUIRED)
# NOTA: Ya NO buscamos ImGui aquí con find_package

# Definir ruta de los fuentes
set(SRC_DIR "${CMAKE_SOURCE_DIR}/glut_simple_example/glut_simple_example")
set(IMGUI_DIR "${SRC_DIR}/backends/imgui")

# --------------------------------------------------------
# DEFINICIÓN DE FUENTES DE IMGUI (MANUAL)
# --------------------------------------------------------
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    # Los Backends
    ${IMGUI_DIR}/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/imgui_impl_opengl3.cpp
)

# Source files del Motor
file(GLOB SOURCES
    ${SRC_DIR}/*.cpp
)

# Header files del Motor
file(GLOB HEADERS
    ${SRC_DIR}/*.h
)

# Create executable (Incluimos las fuentes de ImGui aquí)
add_executable(Motor ${SOURCES} ${HEADERS} ${IMGUI_SOURCES})

# --------------------------------------------------------
# VINCULANDO LIBRERÍAS
# --------------------------------------------------------
target_link_libraries(Motor PRIVATE 
    SDL3::SDL3
    glm::glm
    assimp::assimp
    GLEW::GLEW
    ${IL_LIBRARIES}
    ${ILU_LIBRARIES}
    fmt::fmt
    # NOTA: Quitamos imgui::imgui porque ahora es parte del código fuente
)

target_include_directories(Motor PRIVATE 
    ${IL_INCLUDE_DIR}
    ${SRC_DIR}
    ${IMGUI_DIR} 
)

# Copy Assets folder to build directory
add_custom_command(TARGET Motor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/Assets                
    $<TARGET_FILE_DIR:Motor>/Assets
    COMMENT "Copying Assets folder to build directory"
)

if(WIN32)
    add_custom_command(TARGET Motor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Remember to copy DevIL.dll and ILU.dll to executable directory if not handled by vcpkg"
    )
endif()