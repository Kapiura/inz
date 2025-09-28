#include "Cloth.hpp"
#include "AABB.hpp"
#include "Ray.hpp"

#include <algorithm>
#include <cmath>

AABB Mass::getAABB() const
{
    float radius = 0.2f; // catch radius
    return AABB(position - glm::vec3(radius), position + glm::vec3(radius));
}

Cloth::Cloth(float width, float height, int resX, int resY, float floorY) : floorY(floorY)
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
    // draw all masses
    shader.setVec3("color", glm::vec3(1.0f, 0.0f, 0.0f));
    glPointSize(5.0f);
    glBindVertexArray(VAO_masses);
    glDrawArrays(GL_POINTS, 0, massesVertices.size() / 3);

    // selected mass is highlighted
    if (selectedMassIndex != -1)
    {
        shader.setVec3("color", glm::vec3(1.0f, 1.0f, 0.0f));
        glPointSize(10.0f);
        glDrawArrays(GL_POINTS, selectedMassIndex, 1);
    }

    glBindVertexArray(0);
}

void Cloth::update(float dt)
{
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

int Cloth::pickMassPoint(const Ray &ray)
{
    float closestT = std::numeric_limits<float>::max();
    int closestIndex = -1;

    for (int i = 0; i < masses.size(); ++i)
    {
        float t;
        AABB aabb = masses[i].getAABB();
        if (aabb.intersect(ray, t))
        {
            if (t < closestT)
            {
                closestT = t;
                closestIndex = i;
            }
        }
    }

    selectedMassIndex = closestIndex;
    return closestIndex;
}

void Cloth::setMassPosition(int index, const glm::vec3 &position)
{
    if (index >= 0 && index < masses.size())
    {
        glm::vec3 clampedPos = position;
        clampedPos.x = glm::clamp(clampedPos.x, -10.0f, 10.0f);
        clampedPos.z = glm::clamp(clampedPos.z, -10.0f, 10.0f);
        clampedPos.y = glm::max(clampedPos.y, floorY);

        masses[index].position = clampedPos;
        // mass velocity reset
        masses[index].velocity = glm::vec3(0.0f);
    }
}

void Cloth::releaseMassPoint(int index)
{
    if (index == selectedMassIndex)
    {
        selectedMassIndex = -1;
    }
}