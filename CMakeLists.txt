cmake_minimum_required(VERSION 3.16)
project(ClothSim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(imgui
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    external/imgui
    external/imgui/backends
)

add_library(glad src/glad.c)
target_include_directories(glad PUBLIC include)

add_executable(my_opengl_in_test
    src/main.cpp
    src/Camera.cpp
    src/Shader.cpp
    src/Cloth.cpp
)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)

target_include_directories(my_opengl_in_test PRIVATE include)

target_link_libraries(my_opengl_in_test
    PRIVATE
        glad
        imgui
        glfw
        OpenGL::GL
)
