#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

#include "Shader.hpp"

struct Point
{
    glm::vec3 position;
    glm::vec3 prevPosition;
    glm::vec3 velocity;
    bool isFixed;
    bool isMovable;

    Point(glm::vec3 pos, bool fixed)
        : position(pos), prevPosition(pos), velocity(0.0f), isFixed(fixed), isMovable(!fixed) {};
};

struct Spring
{
    int pointA;
    int pointB;
    float restLen;
    float stiffness;
    bool isMovable;

    Spring(int a, int b, float length, float stiff = 2.0f)
        : pointA(a), pointB(b), restLen(length), stiffness(stiff), isMovable(true)
    {
    }
};
class Cloth
{
  public:
    Cloth(float width, float height, int resX, int resY);
    void draw(Shader &shader);
    void update(float dt);

    void setPointFixed(int x, int y, bool fixed);
    void setSpringMoveable(int springId, bool moveable);
    void applyForceToPoint(int pointId, const glm::vec3 &force);

  private:
    int resX, resY;
    float width, height;

    void rebuildGraphicsData();
    void applyConsts();

    std::vector<Point> points;
    std::vector<Spring> springs;

    std::vector<float> pointVertices;
    std::vector<float> lineVertices;

    unsigned int VAO_points = 0, VBO_points = 0;
    unsigned int VAO_lines = 0, VBO_lines = 0;
};