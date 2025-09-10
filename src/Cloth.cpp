#include "Cloth.hpp"
#include <algorithm>

#include <cmath>

glm::vec3 midpoint(const glm::vec3& a, const glm::vec3& b)
{
    return (a + b) * 0.5f;
}

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

    // pion
    for (int y = 0; y < resY; y++)
    {
        for (int x = 0; x < resX - 1; x++)
        {
            int idx1 = y * resX + x;
            int idx2 = y * resX + (x + 1);
            float length = glm::distance(points[idx1].position, points[idx2].position);
            glm::vec3 mp = midpoint(points[idx1].position, points[idx2].position);
            springs.emplace_back(idx1, idx2, length, mp);
        }
    }

    // poziom
    for (int y = 0; y < resY - 1; y++)
    {
        for (int x = 0; x < resX; x++)
        {
            int idx1 = y * resX + x;
            int idx2 = (y + 1) * resX + x;
            float length = glm::distance(points[idx1].position, points[idx2].position);
            glm::vec3 mp = midpoint(points[idx1].position, points[idx2].position);
            springs.emplace_back(idx1, idx2, length, mp);
        }
    }

    // ukos
    for (int y = 0; y < resY - 1; y++)
    {
        for (int x = 0; x < resX - 1; x++)
        {
            int idx1 = y * resX + x;
            int idx2 = (y + 1) * resX + (x + 1);
            float length = glm::distance(points[idx1].position, points[idx2].position);
            glm::vec3 mp = midpoint(points[idx1].position, points[idx2].position);
            springs.emplace_back(idx1, idx2, length, mp);
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

    glGenVertexArrays(1, &VAO_lines);
    glGenBuffers(1, &VBO_lines);

    glBindVertexArray(VAO_lines);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_lines);
    glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float), lineVertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
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

    for (const auto &spring : springs)
    {
        const auto &posA = points[spring.pointA].position;
        const auto &posB = points[spring.pointB].position;

        lineVertices.push_back(posA.x);
        lineVertices.push_back(posA.y);
        lineVertices.push_back(posA.z);

        lineVertices.push_back(posB.x);
        lineVertices.push_back(posB.y);
        lineVertices.push_back(posB.z);
    }
}

void Cloth::update(float dt)
{
    float floorY = 0.0f;
    float gravityForce = gravity / pointWeight;

    // gravity
    for(auto& point: points)
    {
        if (!point.isFixed)
        point.applyForce(glm::vec3(0.0f, gravityForce, 0.0f));

        point.update(dt);
    }

    for (auto &spring : springs)
    {
        Point &pointA = points[spring.pointA];
        Point &pointB = points[spring.pointB];

        glm::vec3 delta = pointB.position - pointA.position;
        glm::vec3 mp = midpoint(pointA.position, pointB.position);
        float currentLength = glm::length(delta);
        float maxStretchedLength = spring.restLen * 1.5f;

        if (currentLength > maxStretchedLength)
        {
            glm::vec3 correctionDirection = delta / currentLength;
            float overStretch = currentLength - maxStretchedLength;

            if (!pointA.isFixed)
            {
                pointA.position += correctionDirection * 0.5f * overStretch;
            }
            if (!pointB.isFixed)
            {
                pointB.position -= correctionDirection * 0.5f * overStretch;
            }
        }
        else
        {
            float difference = currentLength - spring.restLen;
            if (difference != 0.0f)
            {
                glm::vec3 correctionDirection = delta / currentLength;
                float correctionForce = difference * spring.stiffness * dt;

                if (!pointA.isFixed)
                {
                    pointA.position += correctionDirection * 0.5f * correctionForce;
                }
                if (!pointB.isFixed)
                {
                    pointB.position -= correctionDirection * 0.5f * correctionForce;
                }
            }
        }
        spring.midpoint = mp;
    }

    rebuildGraphicsData();
    glBindBuffer(GL_ARRAY_BUFFER, VBO_points);
    glBufferSubData(GL_ARRAY_BUFFER, 0, pointVertices.size() * sizeof(float), pointVertices.data());

    glBindBuffer(GL_ARRAY_BUFFER, VBO_points);
    glBufferData(GL_ARRAY_BUFFER, pointVertices.size() * sizeof(float), pointVertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_lines);
    glBufferSubData(GL_ARRAY_BUFFER, 0, lineVertices.size() * sizeof(float), lineVertices.data());

    glBindBuffer(GL_ARRAY_BUFFER, VBO_lines);
    glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float), lineVertices.data(), GL_DYNAMIC_DRAW);
}

void Cloth::draw(Shader &shader)
{
    shader.setVec3("color", glm::vec3(1.0f, 0.0f, 0.0f));
    glPointSize(5.0f);
    glBindVertexArray(VAO_points);
    glDrawArrays(GL_POINTS, 0, pointVertices.size() / 3);

    shader.setVec3("color", glm::vec3(0.0f, 1.0f, 0.0f));
    glBindVertexArray(VAO_lines);
    glDrawArrays(GL_LINES, 0, lineVertices.size() / 3);

    glBindVertexArray(0);
}

void Point::applyForce(glm::vec3&& force)
{
    acceleration += force;
}

void Point::update(float dt)
{
    // verlet
    glm::vec3 velocity = position - prevPosition;
    prevPosition = position;
    position += velocity + acceleration * dt * dt;
    acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
}

void Cloth::tear(const glm::vec3 &rayDir, const glm::vec3 &rayOrigin)
{
    int closestIdx = -1;
    float minDist = std::numeric_limits<float>::max();

    for (int i = 0; i < points.size(); i++)
    {
        glm::vec3 p = points[i].position;
        glm::vec3 v = p - rayOrigin;
        float t = glm::dot(v, rayDir);
        glm::vec3 closestOnRay = rayOrigin + t * rayDir;

        float dist = glm::length(p - closestOnRay);
        if (dist < minDist)
        {
            minDist = dist;
            closestIdx = i;
        }
    }

    if (closestIdx == -1)
        return;

    points.erase(points.begin() + closestIdx);

    springs.erase(
        std::remove_if(springs.begin(), springs.end(),
                       [closestIdx](const Spring &s)
                       {
                           return s.pointA == closestIdx || s.pointB == closestIdx;
                       }),
        springs.end());

    for (auto &s : springs)
    {
        if (s.pointA > closestIdx) s.pointA--;
        if (s.pointB > closestIdx) s.pointB--;
    }
}