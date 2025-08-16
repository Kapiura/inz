#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

#include "Shader.hpp"

class Cloth
{
  public:
    Cloth(float width, float height, int resX, int resY);
    void draw(Shader &shader);
    void update(float dt);

  private:
    int resX, resY;
    float width, height;

    std::vector<float> pointVertices;
    std::vector<float> lineVertices;

    unsigned int VAO_points = 0, VBO_points = 0;
    unsigned int VAO_lines = 0, VBO_lines = 0;
};
