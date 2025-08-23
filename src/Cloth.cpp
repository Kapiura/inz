#include "Cloth.hpp"

#include <cmath>

Cloth::Cloth(float width, float height, int resX, int resY)
{
    VAO_points = 0, VBO_points = 0;
    VAO_lines = 0, VBO_lines = 0;

    points.reserve(resX * resY);
    springs.reserve((resX - 1) * resY + resX * (resY - 1));

    for (int y = 0; y < resY; y++)
    {
        for (int x = 0; x < resX; x++)
        {
            float xpos = (x / float(resX - 1)) * width - width / 2.0f;
            float ypos = 5.0f - (y / float(resY - 1)) * height;
            float zpos = 0.0f;

            bool isFixed = (y == 0);
            points.emplace_back(glm::vec3(xpos, ypos, zpos), isFixed);
        }
    }

    rebuildGraphicsData();

    // VAO/VBO dla punktów
    glGenVertexArrays(1, &VAO_points);
    glGenBuffers(1, &VBO_points);

    glBindVertexArray(VAO_points);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_points);
    glBufferData(GL_ARRAY_BUFFER, pointVertices.size() * sizeof(float), pointVertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Cloth::rebuildGraphicsData()
{
    pointVertices.clear();
    lineVertices.clear();

    for (const auto &point : points)
    {
        pointVertices.push_back(point.position.x);
        pointVertices.push_back(point.position.y);
        pointVertices.push_back(point.position.z);
    }
}

void Cloth::update(float dt)
{
    float floorY = 0.0f;

    for (auto &p : points)
    {
        if (!p.isFixed)
        {
            p.position.y -= 0.01f;
            if (p.position.y < floorY)
                p.position.y = floorY;
        }
    }

    rebuildGraphicsData();
    glBindBuffer(GL_ARRAY_BUFFER, VBO_points);
    glBufferSubData(GL_ARRAY_BUFFER, 0, pointVertices.size() * sizeof(float), pointVertices.data());

    glBindBuffer(GL_ARRAY_BUFFER, VBO_points);
    glBufferData(GL_ARRAY_BUFFER, pointVertices.size() * sizeof(float), pointVertices.data(), GL_DYNAMIC_DRAW);
}

void Cloth::draw(Shader &shader)
{
    shader.setVec3("color", glm::vec3(1.0f, 0.0f, 0.0f));
    glPointSize(5.0f);
    glBindVertexArray(VAO_points);
    glDrawArrays(GL_POINTS, 0, pointVertices.size() / 3);
    glBindVertexArray(0);
}