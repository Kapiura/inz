#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

#include "Shader.hpp"

struct Mass
{
    glm::vec3 position;
    glm::vec3 prevPosition;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    glm::vec3 force;
    float mass;
    bool fixed;

    Mass(const glm::vec3 &pos, float m, bool fix = false)
        : position(pos), prevPosition(pos), velocity(0.0f), acceleration(0.0f), force(0.0f), mass(m), fixed(fix)
    {
    }

    void update(float dt);
    void applyForce(glm::vec3 &&force);
};

struct Spring
{
    int a, b;
    float restLength;
    float stiffness;
    float damping;
    glm::vec3 midpoint;

    Spring(int a, int b, float rest, float k, float d)
        : a(a), b(b), restLength(rest), stiffness(k), damping(d), midpoint(0.0f)
    {
    }
};

class Cloth
{
  public:
    Cloth(float width, float height, int resX, int resY);
    void draw(Shader &shader);
    void update(float dt);

    void satisfy();

  private:
    int resX, resY;
    float width, height;

    void rebuildGraphicsData();
    void applyConsts();

    std::vector<Mass> masses;
    std::vector<Spring> springs;

    std::vector<float> massesVertices;
    std::vector<float> lineVertices;

    unsigned int VAO_masses = 0, VBO_masses = 0;
    unsigned int VAO_lines = 0, VBO_lines = 0;

    const float gravity = -9.81f;
};