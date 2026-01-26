#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// Manages GPU shaders programs for OpenGL rendering
class Shader
{
  public:
    unsigned int ID;

    Shader(const char *vertexPath, const char *fragmentPath);

    void use();

    // Single value
    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    // 2D values
    void setVec2(const std::string &name, float x, float y) const;
    void setVec2(const std::string &name, const glm::vec2 &value) const;
    // 3D values
    void setVec3(const std::string &name, float x, float y, float z) const;
    void setVec3(const std::string &name, const glm::vec3 &value) const;
    // 4D values
    void setVec4(const std::string &name, float x, float y, float z, float w) const;
    void setVec4(const std::string &name, const glm::vec4 &value) const;
    // Set matrices
    void setMat2(const std::string &name, const glm::mat2 &mat) const;
    void setMat3(const std::string &name, const glm::mat3 &mat) const;
    void setMat4(const std::string &name, const glm::mat4 &mat) const;
};