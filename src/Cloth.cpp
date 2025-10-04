#include "Cloth.hpp"
#include "AABB.hpp"
#include "Ray.hpp"

#include <algorithm>
#include <cmath>

glm::vec3 midpoint(const glm::vec3 &a, const glm::vec3 &b)
{
    return (a + b) * 0.5f;
}

AABB Mass::getAABB() const
{
    float radius = 0.2f;
    return AABB(position - glm::vec3(radius), position + glm::vec3(radius));
}

Cloth::Cloth(float width, float height, int resX, int resY, float floorY) : floorY(floorY)
{
    VAO_masses = 0, VBO_masses = 0;
    VAO_lines = 0, VBO_lines = 0;

    masses.reserve(resX * resY);
    springs.reserve((resX - 1) * resY + resX * (resY - 1) + (resX - 1) * (resY - 1));

    const float massValue = 1.0f;

    // spring params
    float structuralStiff = 80.0f;
    float structuralDamping = 0.5f;
    float shearStiff = 60.0f;
    float shearDamping = 0.3f;

    // masses mesh
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

    // poziom
    for (int y = 0; y < resY; y++)
    {
        for (int x = 0; x < resX - 1; x++)
        {
            int idx1 = y * resX + x;
            int idx2 = y * resX + (x + 1);
            float length = glm::distance(masses[idx1].position, masses[idx2].position);
            glm::vec3 mp = midpoint(masses[idx1].position, masses[idx2].position);
            springs.emplace_back(idx1, idx2, length, mp, structuralStiff, structuralDamping);
        }
    }

    // pion
    for (int y = 0; y < resY - 1; y++)
    {
        for (int x = 0; x < resX; x++)
        {
            int idx1 = y * resX + x;
            int idx2 = (y + 1) * resX + x;
            float length = glm::distance(masses[idx1].position, masses[idx2].position);
            glm::vec3 mp = midpoint(masses[idx1].position, masses[idx2].position);
            springs.emplace_back(idx1, idx2, length, mp, structuralStiff, structuralDamping);
        }
    }

    // ukos
    for (int y = 0; y < resY - 1; y++)
    {
        for (int x = 0; x < resX - 1; x++)
        {
            int idx1 = y * resX + x;
            int idx2 = (y + 1) * resX + (x + 1);
            float length = glm::distance(masses[idx1].position, masses[idx2].position);
            glm::vec3 mp = midpoint(masses[idx1].position, masses[idx2].position);
            springs.emplace_back(idx1, idx2, length, mp, shearStiff, shearDamping);

            idx1 = y * resX + (x + 1);
            idx2 = (y + 1) * resX + x;
            length = glm::distance(masses[idx1].position, masses[idx2].position);
            mp = midpoint(masses[idx1].position, masses[idx2].position);
            springs.emplace_back(idx1, idx2, length, mp, shearStiff, shearDamping);
        }
    }

    rebuildGraphicsData();

    // VAO and VBO for masses
    glGenVertexArrays(1, &VAO_masses);
    glGenBuffers(1, &VBO_masses);

    glBindVertexArray(VAO_masses);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_masses);
    glBufferData(GL_ARRAY_BUFFER, massesVertices.size() * sizeof(float), massesVertices.data(), GL_DYNAMIC_DRAW);
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
    // rebuild masses
    massesVertices.clear();
    for (const auto &mass : masses)
    {
        massesVertices.push_back(mass.position.x);
        massesVertices.push_back(mass.position.y);
        massesVertices.push_back(mass.position.z);
    }

    // rebuild spring lines
    lineVertices.clear();
    for (const auto &spring : springs)
    {
        lineVertices.push_back(masses[spring.a].position.x);
        lineVertices.push_back(masses[spring.a].position.y);
        lineVertices.push_back(masses[spring.a].position.z);

        lineVertices.push_back(masses[spring.b].position.x);
        lineVertices.push_back(masses[spring.b].position.y);
        lineVertices.push_back(masses[spring.b].position.z);
    }

    if (VAO_lines != 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, VBO_lines);
        glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float), lineVertices.data(), GL_DYNAMIC_DRAW);
    }
}

void Cloth::draw(Shader &shader)
{
    // draw springs as lines
    shader.setVec3("color", glm::vec3(0.0f, 0.0f, 1.0f));
    glBindVertexArray(VAO_lines);
    glDrawArrays(GL_LINES, 0, lineVertices.size() / 3);

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
    for (auto &mass : masses)
    {
        if (!mass.fixed)
            mass.applyForce(glm::vec3(0.0f, gravity, 0.0f));
        mass.update(dt);
    }

    for (int iter = 0; iter < 3; ++iter)
    {
        for (auto &spring : springs)
        {
            Mass &massA = masses[spring.a];
            Mass &massB = masses[spring.b];

            glm::vec3 delta = massB.position - massA.position;
            float currentLength = glm::length(delta);

            if (currentLength > 0.0f)
            {
                float maxStretchRatio = 1.5f;
                float maxLength = spring.restLength * maxStretchRatio;

                if (currentLength > maxLength)
                {
                    glm::vec3 direction = delta / currentLength;
                    float overStretch = currentLength - maxLength;

                    float correction = overStretch * 0.5f;

                    if (!massA.fixed)
                        massA.position += direction * correction;
                    if (!massB.fixed)
                        massB.position -= direction * correction;
                }
                else
                {
                    float difference = currentLength - spring.restLength;
                    glm::vec3 correction = (delta / currentLength) * difference * 0.5f;

                    float correctionFactor = 0.1f * (spring.stiffness / 100.0f);
                    if (!massA.fixed)
                        massA.position += correction * correctionFactor;
                    if (!massB.fixed)
                        massB.position -= correction * correctionFactor;
                }
            }
        }
    }

    // floor collision
    for (auto &mass : masses)
    {
        if (mass.position.y < floorY)
        {
            mass.position.y = floorY;
            glm::vec3 velocity = mass.position - mass.prevPosition;
            mass.prevPosition = mass.position - velocity * 0.3f;
        }
    }

    rebuildGraphicsData();

    glBindBuffer(GL_ARRAY_BUFFER, VBO_masses);
    glBufferSubData(GL_ARRAY_BUFFER, 0, massesVertices.size() * sizeof(float), massesVertices.data());

    if (VAO_lines != 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, VBO_lines);
        glBufferSubData(GL_ARRAY_BUFFER, 0, lineVertices.size() * sizeof(float), lineVertices.data());
    }
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
        masses[index].velocity = glm::vec3(0.0f);

        checkTearingAroundPoint(index);
    }
}

void Cloth::checkTearingAroundPoint(int massIndex)
{
    const float tearThreshold = 20.0f;

    for (auto it = springs.begin(); it != springs.end();)
    {
        if (it->a == massIndex || it->b == massIndex)
        {
            Mass &massA = masses[it->a];
            Mass &massB = masses[it->b];

            glm::vec3 delta = massB.position - massA.position;
            float currentLength = glm::length(delta);
            float stretchRatio = currentLength / it->restLength;

            if (stretchRatio > tearThreshold)
            {
                std::cout << "Spring torn! Stretch ratio: " << stretchRatio << std::endl;
                it = springs.erase(it);
                continue;
            }
        }
        ++it;
    }
}

void Cloth::releaseMassPoint(int index)
{
    if (index == selectedMassIndex)
    {
        selectedMassIndex = -1;
    }
}

bool Cloth::springIntersectsSegment(const Spring &spring, const glm::vec3 &segmentStart, const glm::vec3 &segmentEnd,
                                    glm::vec3 &intersectionPoint)
{
    const Mass &massA = masses[spring.a];
    const Mass &massB = masses[spring.b];

    glm::vec3 springStart = massA.position;
    glm::vec3 springEnd = massB.position;
    glm::vec3 segmentDir = segmentEnd - segmentStart;
    float segmentLength = glm::length(segmentDir);

    if (segmentLength < 0.001f)
        return false;
    segmentDir = segmentDir / segmentLength;

    glm::vec3 springCenter = (springStart + springEnd) * 0.5f;
    glm::vec3 toCenter = springCenter - segmentStart;
    float projection = glm::dot(toCenter, segmentDir);
    projection = glm::clamp(projection, 0.0f, segmentLength);
    glm::vec3 closestPointOnSegment = segmentStart + segmentDir * projection;
    float distance = glm::length(springCenter - closestPointOnSegment);
    float threshold = 0.3f;

    if (distance <= threshold)
    {
        intersectionPoint = springCenter;
        return true;
    }

    return false;
}

void Cloth::cutSpringsWithRay(const Ray &ray, const glm::vec3 &previousMousePos)
{
    glm::vec3 currentMouseWorldPos;

    float avgClothHeight = 0.0f;
    for (const auto &mass : masses)
    {
        avgClothHeight += mass.position.y;
    }

    avgClothHeight /= masses.size();
    if (std::abs(ray.Direction().y) > 0.0001f)
    {
        float t = (avgClothHeight - ray.Origin().y) / ray.Direction().y;
        if (t > 0.1f && t < 50.0f)
        {
            currentMouseWorldPos = ray.At(t);
        }
        else
        {
            currentMouseWorldPos = ray.Origin() + ray.Direction() * 10.0f;
        }
    }
    else
    {
        currentMouseWorldPos = ray.Origin() + ray.Direction() * 10.0f;
    }

    std::vector<int> springsToCut;
    for (int i = 0; i < springs.size(); ++i)
    {
        glm::vec3 intersectionPoint;
        if (springIntersectsSegment(springs[i], previousMousePos, currentMouseWorldPos, intersectionPoint))
        {
            springsToCut.push_back(i);
            std::cout << "Cut spring " << i << " at position (" << intersectionPoint.x << ", " << intersectionPoint.y
                      << ", " << intersectionPoint.z << ")" << std::endl;
        }
    }

    if (!springsToCut.empty())
    {
        std::cout << "Cutting " << springsToCut.size() << " springs" << std::endl;

        for (int i = springsToCut.size() - 1; i >= 0; --i)
        {
            int index = springsToCut[i];
            springs.erase(springs.begin() + index);
        }

        rebuildGraphicsData();
    }
}