#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

#include "Shader.hpp"

struct Point
{
    glm::vec3 position;
    glm::vec3 prevPosition;
    glm::vec3 acceleration;
    bool isFixed;
    bool isMovable;

    Point(glm::vec3 pos, bool fixed)
        : position(pos), prevPosition(pos), acceleration(0.0f), isFixed(fixed), isMovable(!fixed) {};

    void update(float dt);
    void applyForce(glm::vec3&& force);
};

struct Spring
{
    int pointA;
    int pointB;
    float restLen;
    float stiffness;
    glm::vec3 midpoint;

Spring(int a, int b, float length, glm::vec3 midpoint, float stiff = 2.0f)
    : pointA(a), pointB(b), restLen(length), stiffness(stiff), midpoint(midpoint)
{}

};
class Cloth
{
  public:
    Cloth(float width, float height, int resX, int resY);
    void draw(Shader &shader);
    void update(float dt);

    void satisfy();
    void tear(const glm::vec3 &rayDir, const glm::vec3 &rayOrigin);

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

    const float gravity = -9.81f;
    float pointWeight = 100.0f;
};