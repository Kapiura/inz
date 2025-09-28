cmake_minimum_required(VERSION 3.16)
project(ClothSim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
        glfw
        OpenGL::GL
)
