cmake_minimum_required(VERSION 3.0.0)

project(my_opengl_in_test VERSION 0.1.0)

add_executable(my_opengl_in_test 
  src/main.cpp
  src/glad.c)

target_link_libraries(my_opengl_in_test glfw)
