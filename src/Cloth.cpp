#include "Cloth.hpp"

#include <algorithm>
#include <cmath>

Cloth::Cloth(float width, float height, int resX, int resY)
{
    // vao vbo
    VAO_masses = 0, VBO_masses = 0;
    VAO_lines = 0, VBO_lines = 0;

    masses.reserve(resX * resY);
    springs.reserve((resX - 1) * resY + resX * (resY - 1));

    const int massValue = 50.0f;

    // generate masses mesh
    for (int y = 0; y < resY; y++)
    {
        for (int x = 0; x < resX; x++)
        {
            float xpos = (x / float(resX - 1)) * width - width / 2.0f;
            float ypos = 5.0f - (y / float(resY - 1)) * height;
            float zpos = 0.0f;

            bool isFixed = (y == 0);
            masses.emplace_back(glm::vec3(xpos, ypos, zpos), massValue, isFixed);
        }
    }

    // rebuild data
    rebuildGraphicsData();

    // VAO and VBO for masses
    glGenVertexArrays(1, &VAO_masses);
    glGenBuffers(1, &VBO_masses);

    glBindVertexArray(VAO_masses);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_masses);
    glBufferData(GL_ARRAY_BUFFER, massesVertices.size() * sizeof(float), massesVertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
}

void Cloth::rebuildGraphicsData()
{
    // rebuild masses
    massesVertices.clear();
    for (const auto &mass : masses)
    {
        massesVertices.push_back(mass.position.x);
        massesVertices.push_back(mass.position.y);
        massesVertices.push_back(mass.position.z);
    }
}

void Cloth::draw(Shader &shader)
{
    // draw masses
    shader.setVec3("color", glm::vec3(1.0f, 0.0f, 0.0f));
    glPointSize(5.0f);
    glBindVertexArray(VAO_masses);
    glDrawArrays(GL_POINTS, 0, massesVertices.size() / 3);

    glBindVertexArray(0);
}

void Cloth::update(float dt)
{
    float floorY = 0.0f;

    // gravity
    for (auto &mass : masses)
    {
        if (mass.position.y < floorY)
            mass.position.y = floorY;
        if (!mass.fixed)
            mass.applyForce(glm::vec3(0.0f, gravity, 0.0f));
        mass.update(dt);
    }

    rebuildGraphicsData();
    glBindBuffer(GL_ARRAY_BUFFER, VBO_masses);
    glBufferSubData(GL_ARRAY_BUFFER, 0, massesVertices.size() * sizeof(float), massesVertices.data());

    glBindBuffer(GL_ARRAY_BUFFER, VBO_masses);
    glBufferData(GL_ARRAY_BUFFER, massesVertices.size() * sizeof(float), massesVertices.data(), GL_DYNAMIC_DRAW);
}

void Mass::update(float dt)
{
    // verlet method
    glm::vec3 velocity = position - prevPosition;
    prevPosition = position;
    position += velocity + acceleration * dt * dt;
    acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
}
void Mass::applyForce(glm::vec3 &&force)
{
    acceleration += force / mass;
}