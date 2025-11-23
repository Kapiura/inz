cmake_minimum_required(VERSION 3.16)
project(ClothSim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG master
)

FetchContent_MakeAvailable(imgui)

set(IMGUI_DIR ${imgui_SOURCE_DIR})

set(IMGUI_SRC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_library(imgui_lib ${IMGUI_SRC})

target_include_directories(imgui_lib PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

add_library(glad src/glad.c)
target_include_directories(glad PUBLIC include)

add_executable(my_opengl_in_test
    src/main.cpp
    src/AABB.cpp
    src/Camera.cpp
    src/Ray.cpp
    src/Shader.cpp
    src/Cloth.cpp
    src/Texture.cpp
    src/Skybox.cpp
    src/Force.cpp
)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)

target_include_directories(my_opengl_in_test PRIVATE include)

target_link_libraries(my_opengl_in_test
    PRIVATE
        glad
        glfw
        OpenGL::GL
        imgui_lib
)
