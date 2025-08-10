#pragma once

#include <array>
#include <glm/glm.hpp>
#include <vector>

struct MassPoint
{
    glm::vec3 position;
    glm::vec3 prevPosition;
    glm::vec3 acceleration;
    bool pinned = false;
};
struct Triangle
{
    std::array<float, 3> indicies; // trujkont
    std::array<glm::vec2, 3> uv;
};

class Cloth
{
  public:
    std::vector<MassPoint> points;   // lista punktow
    std::vector<Triangle> triangles; // po 2 na kazdy kwadrat

    Cloth(float clothWidth, float clothHeight, int resX, int resY);

    void simulate(float dt);
    void draw();

  private:
    float width;
    float height;
    int resX;
    int resY;
};