#include "Cloth.hpp"

void Cloth::draw(Shader &shader)
{
    shader.setVec3("color", glm::vec3(1.0f, 0.0f, 0.0f));
    glPointSize(5.0f);
    glBindVertexArray(VAO_points);
    glDrawArrays(GL_POINTS, 0, pointVertices.size() / 3);

    shader.setVec3("color", glm::vec3(0.0f, 1.0f, 0.0f));
    glBindVertexArray(VAO_lines);
    glDrawArrays(GL_LINES, 0, lineVertices.size() / 3);
}

Cloth::Cloth(float width, float height, int resX, int resY)
{
    VAO_points = 0, VBO_points = 0;
    VAO_lines = 0, VBO_lines = 0;
    for (int y = 0; y < resY; y++)
    {
        for (int x = 0; x < resX; x++)
        {
            float xpos = (x / float(resX - 1)) * width;
            float ypos = (y / float(resY - 1)) * height;
            float zpos = 0.0f;

            pointVertices.push_back(xpos);
            pointVertices.push_back(ypos);
            pointVertices.push_back(zpos);

            if (x < resX - 1)
            {
                lineVertices.push_back(xpos);
                lineVertices.push_back(ypos);
                lineVertices.push_back(zpos);
                lineVertices.push_back((x + 1) / float(resX - 1) * width);
                lineVertices.push_back(ypos);
                lineVertices.push_back(zpos);
            }

            if (y < resY - 1)
            {
                lineVertices.push_back(xpos);
                lineVertices.push_back(ypos);
                lineVertices.push_back(zpos);
                lineVertices.push_back(xpos);
                lineVertices.push_back((y + 1) / float(resY - 1) * height);
                lineVertices.push_back(zpos);
            }
        }
    }

    // VAO / VBO points
    glGenVertexArrays(1, &VAO_points);
    glGenBuffers(1, &VBO_points);
    glBindVertexArray(VAO_points);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_points);
    glBufferData(GL_ARRAY_BUFFER, pointVertices.size() * sizeof(float), pointVertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // VAO / VBO lines
    glGenVertexArrays(1, &VAO_lines);
    glGenBuffers(1, &VBO_lines);
    glBindVertexArray(VAO_lines);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_lines);
    glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float), lineVertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
}

void Cloth::update(float dt)
{
    float floorY = 0.0f;

    for (int i = 0; i < pointVertices.size(); i += 3)
    {
        if (pointVertices[i + 1] > floorY)
        {
            pointVertices[i + 1] -= 0.01f;

            if (pointVertices[i + 1] < floorY)
                pointVertices[i + 1] = floorY;
        }
    }

    for (int i = 0; i < lineVertices.size(); i += 6)
    {
        if (lineVertices[i + 1] > floorY)
        {
            lineVertices[i + 1] -= 0.01f;
            lineVertices[i + 4] -= 0.01f;

            if (lineVertices[i + 1] < floorY)
                lineVertices[i + 1] = floorY;
            if (lineVertices[i + 4] < floorY)
                lineVertices[i + 4] = floorY;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO_points);
    glBufferSubData(GL_ARRAY_BUFFER, 0, pointVertices.size() * sizeof(float), pointVertices.data());

    glBindBuffer(GL_ARRAY_BUFFER, VBO_lines);
    glBufferSubData(GL_ARRAY_BUFFER, 0, lineVertices.size() * sizeof(float), lineVertices.data());
}