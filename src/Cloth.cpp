#include "Cloth.hpp"
#include "AABB.hpp"
#include "AnalysisData.hpp"
#include "Object.hpp"
#include "Ray.hpp"

#include <algorithm>
#include <cmath>
#include <queue>
#include <set>
#include <stb_image.h>

enum class ClothOrientation;

glm::vec3 midpoint(const glm::vec3 &a, const glm::vec3 &b)
{
    return (a + b) * 0.5f;
}

bool Cloth::areSpringMidpointsConnected(int springA, int springB) const
{
    const Spring &sA = springs[springA];
    const Spring &sB = springs[springB];

    return (sA.a == sB.a || sA.a == sB.b || sA.b == sB.a || sA.b == sB.b);
}

void Cloth::initCloth()
{
    masses.clear();
    springs.clear();
    massIndexMap.clear();

    forceManager.clear();
    forceManager.addForce<GravityForce>(-9.81f);
    forceManager.addForce<WindForce>(glm::vec3(1.0f, 0.0f, 0.0f), 5.0f);

    simulationTime = 0.0f;

    masses.reserve(resX * resY);
    springs.reserve((resX - 1) * resY + resX * (resY - 1) + (resX - 1) * (resY - 1));
    massIndexMap.resize(resX * resY);

    const float massValue = defaultMass;

    float structuralStiff = defaultStructuralStiffness;
    float structuralDamping = defaultStructuralDamping;
    float shearStiff = defaultShearStiffness;
    float shearDamping = defaultShearDamping;
    float bendingStiff = defaultBendingStiffness;
    float bendingDamping = defaultBendingDamping;

    for (int y = 0; y < resY; y++)
    {
        for (int x = 0; x < resX; x++)
        {
            glm::vec3 pos;
            bool isFixed;

            if (currentOrientation == ClothOrientation::VERTICAL)
            {
                float xpos = (x / float(resX - 1)) * width - width / 2.0f;
                float ypos = 5.0f - (y / float(resY - 1)) * height;
                float zpos = 0.0f;

                pos = glm::vec3(xpos, ypos, zpos);
                isFixed = (y == 0);
            }
            else
            {
                float xpos = (x / float(resX - 1)) * width - width / 2.0f;
                float ypos = 5.0f;
                float zpos = (y / float(resY - 1)) * height - height / 2.0f;

                pos = glm::vec3(xpos, ypos, zpos);

                isFixed = (x == 0) || (x == resX - 1) || (y == 0) || (y == resY - 1);
            }

            float u = x / float(resX - 1);
            float v = y / float(resY - 1);

            masses.emplace_back(pos, massValue, isFixed, glm::vec2(u, v));

            if (!isFixed)
            {
                float offsetScale = 0.001f;
                glm::vec3 offset((x % 2 == 0 ? 1.0f : -1.0f) * offsetScale, -offsetScale * 0.5f,
                                 (y % 2 == 0 ? 1.0f : -1.0f) * offsetScale * 0.5f);
                masses.back().prevPosition = pos - offset;
            }

            int gridIdx = y * resX + x;
            massIndexMap[gridIdx] = gridIdx;
        }
    }

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

    for (int y = 0; y < resY; y++)
    {
        for (int x = 0; x < resX - 2; x++)
        {
            int idx1 = y * resX + x;
            int idx2 = y * resX + (x + 2);
            float length = glm::distance(masses[idx1].position, masses[idx2].position);
            glm::vec3 mp = midpoint(masses[idx1].position, masses[idx2].position);
            springs.emplace_back(idx1, idx2, length, mp, bendingStiff, bendingDamping);
        }
    }

    for (int y = 0; y < resY - 2; y++)
    {
        for (int x = 0; x < resX; x++)
        {
            int idx1 = y * resX + x;
            int idx2 = (y + 2) * resX + x;
            float length = glm::distance(masses[idx1].position, masses[idx2].position);
            glm::vec3 mp = midpoint(masses[idx1].position, masses[idx2].position);
            springs.emplace_back(idx1, idx2, length, mp, bendingStiff, bendingDamping);
        }
    }

    if (textureID == 0)
    {
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        int width, height, nrChannels;
        unsigned char *data = stbi_load("../img/textures/john.jpg", &width, &height, &nrChannels, 0);

        GLenum format;
        GLenum internal;

        if (nrChannels == 1)
        {
            format = GL_RED;
            internal = GL_R8;
        }
        if (nrChannels == 3)
        {
            format = GL_RGB;
            internal = GL_RGB8;
        }
        if (nrChannels == 4)
        {
            format = GL_RGBA;
            internal = GL_RGBA8;
        }

        if (data)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, internal, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        stbi_image_free(data);
    }

    rebuildGraphicsData();
    rebuildTextureData();

    if (VAO_masses == 0)
    {
        glGenVertexArrays(1, &VAO_masses);
        glGenBuffers(1, &VBO_masses);

        glBindVertexArray(VAO_masses);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_masses);
        glBufferData(GL_ARRAY_BUFFER, massesVertices.size() * sizeof(float), massesVertices.data(), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
    }

    if (VAO_lines == 0)
    {
        glGenVertexArrays(1, &VAO_lines);
        glGenBuffers(1, &VBO_lines);

        glBindVertexArray(VAO_lines);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_lines);
        glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float), lineVertices.data(), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
    }

    if (VAO_texture == 0)
    {
        glGenVertexArrays(1, &VAO_texture);
        glGenBuffers(1, &VBO_texture);
        glGenBuffers(1, &EBO_texture);

        glBindVertexArray(VAO_texture);

        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture);
        glBufferData(GL_ARRAY_BUFFER, textureVertices.size() * sizeof(float), textureVertices.data(), GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_texture);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, textureIndices.size() * sizeof(unsigned int), textureIndices.data(),
                     GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }
}

AABB Mass::getAABB() const
{
    float radius = 0.2f;
    return AABB(position - glm::vec3(radius), position + glm::vec3(radius));
}

void Cloth::reset()
{
    selectedMassIndex = -1;
    initCloth();
}

Cloth::Cloth(float width, float height, int resX, int resY, float floorY)
    : width(width), height(height), resX(resX), resY(resY), floorY(floorY + .05f),
      currentOrientation(ClothOrientation::VERTICAL)
{
    VAO_masses = 0, VBO_masses = 0;
    VAO_lines = 0, VBO_lines = 0;
    VAO_texture = 0, VBO_texture = 0, EBO_texture = 0;

    initCloth();
}

void Cloth::rebuildGraphicsData()
{
    massesVertices.clear();
    for (const auto &mass : masses)
    {
        massesVertices.push_back(mass.position.x);
        massesVertices.push_back(mass.position.y);
        massesVertices.push_back(mass.position.z);
    }

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

void Cloth::calculateNormals()
{
    for (auto &mass : masses)
    {
        mass.normal = glm::vec3(0.0f);
    }

    for (int y = 0; y < resY - 1; y++)
    {
        for (int x = 0; x < resX - 1; x++)
        {
            int gridIdx0 = y * resX + x;
            int gridIdx1 = y * resX + (x + 1);
            int gridIdx2 = (y + 1) * resX + x;
            int gridIdx3 = (y + 1) * resX + (x + 1);

            if (gridIdx0 >= massIndexMap.size() || gridIdx1 >= massIndexMap.size() || gridIdx2 >= massIndexMap.size() ||
                gridIdx3 >= massIndexMap.size())
                continue;

            int idx0 = massIndexMap[gridIdx0];
            int idx1 = massIndexMap[gridIdx1];
            int idx2 = massIndexMap[gridIdx2];
            int idx3 = massIndexMap[gridIdx3];

            if (idx0 < 0 || idx1 < 0 || idx2 < 0 || idx3 < 0)
                continue;

            glm::vec3 v0 = masses[idx0].position;
            glm::vec3 v1 = masses[idx1].position;
            glm::vec3 v2 = masses[idx2].position;

            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 normal1 = glm::normalize(glm::cross(edge1, edge2));

            masses[idx0].normal += normal1;
            masses[idx1].normal += normal1;
            masses[idx2].normal += normal1;

            v0 = masses[idx1].position;
            v1 = masses[idx3].position;
            v2 = masses[idx2].position;

            edge1 = v1 - v0;
            edge2 = v2 - v0;
            glm::vec3 normal2 = glm::normalize(glm::cross(edge1, edge2));

            masses[idx1].normal += normal2;
            masses[idx3].normal += normal2;
            masses[idx2].normal += normal2;
        }
    }

    for (auto &mass : masses)
    {
        if (glm::length(mass.normal) > 0.001f)
        {
            mass.normal = glm::normalize(mass.normal);
        }
        else
        {
            mass.normal = glm::vec3(0.0f, 0.0f, 1.0f);
        }
    }
}

void Cloth::rebuildTextureData()
{
    textureVertices.clear();
    textureIndices.clear();

    if (masses.empty())
        return;

    calculateNormals();

    textureVertices.reserve(masses.size() * 8);
    for (const auto &mass : masses)
    {
        textureVertices.push_back(mass.position.x);
        textureVertices.push_back(mass.position.y);
        textureVertices.push_back(mass.position.z);
        textureVertices.push_back(mass.normal.x);
        textureVertices.push_back(mass.normal.y);
        textureVertices.push_back(mass.normal.z);
        textureVertices.push_back(mass.texCoord.x);
        textureVertices.push_back(mass.texCoord.y);
    }

    std::set<std::pair<int, int>> springConnections;
    for (const auto &spring : springs)
    {
        if (spring.a >= masses.size() || spring.b >= masses.size())
            continue;

        int minIdx = std::min(spring.a, spring.b);
        int maxIdx = std::max(spring.a, spring.b);
        springConnections.insert({minIdx, maxIdx});
    }

    auto hasSpring = [&](int a, int b) {
        if (a < 0 || b < 0 || a >= masses.size() || b >= masses.size())
            return false;
        int minIdx = std::min(a, b);
        int maxIdx = std::max(a, b);
        return springConnections.count({minIdx, maxIdx}) > 0;
    };

    textureIndices.reserve((resX - 1) * (resY - 1) * 6);
    for (int y = 0; y < resY - 1; y++)
    {
        for (int x = 0; x < resX - 1; x++)
        {
            int gridIdx0 = y * resX + x;
            int gridIdx1 = y * resX + (x + 1);
            int gridIdx2 = (y + 1) * resX + x;
            int gridIdx3 = (y + 1) * resX + (x + 1);

            if (gridIdx0 >= massIndexMap.size() || gridIdx1 >= massIndexMap.size() || gridIdx2 >= massIndexMap.size() ||
                gridIdx3 >= massIndexMap.size())
                continue;

            int idx0 = massIndexMap[gridIdx0];
            int idx1 = massIndexMap[gridIdx1];
            int idx2 = massIndexMap[gridIdx2];
            int idx3 = massIndexMap[gridIdx3];

            if (idx0 < 0 || idx1 < 0 || idx2 < 0 || idx3 < 0)
                continue;

            int edgeCount1 = 0;
            if (hasSpring(idx0, idx1))
                edgeCount1++;
            if (hasSpring(idx1, idx2))
                edgeCount1++;
            if (hasSpring(idx0, idx2))
                edgeCount1++;

            if (edgeCount1 >= 2)
            {
                textureIndices.push_back(idx0);
                textureIndices.push_back(idx1);
                textureIndices.push_back(idx2);
            }

            int edgeCount2 = 0;
            if (hasSpring(idx1, idx3))
                edgeCount2++;
            if (hasSpring(idx3, idx2))
                edgeCount2++;
            if (hasSpring(idx1, idx2))
                edgeCount2++;

            if (edgeCount2 >= 2)
            {
                textureIndices.push_back(idx1);
                textureIndices.push_back(idx3);
                textureIndices.push_back(idx2);
            }
        }
    }

    if (VAO_texture != 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture);
        glBufferData(GL_ARRAY_BUFFER, textureVertices.size() * sizeof(float), textureVertices.data(), GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_texture);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, textureIndices.size() * sizeof(unsigned int), textureIndices.data(),
                     GL_DYNAMIC_DRAW);
    }
}

void Cloth::draw(Shader &shader)
{
    if (textureVisible)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        shader.setInt("useTexture", 1);

        glBindVertexArray(VAO_texture);
        glDrawElements(GL_TRIANGLES, textureIndices.size(), GL_UNSIGNED_INT, 0);

        shader.setInt("useTexture", 0);
    }

    if (springVisible)
    {
        shader.setVec3("color", glm::vec3(0.0f, 0.0f, 1.0f));
        glBindVertexArray(VAO_lines);
        glDrawArrays(GL_LINES, 0, lineVertices.size() / 3);
    }

    if (massVisible)
    {
        shader.setVec3("color", glm::vec3(1.0f, 0.0f, 0.0f));
        glPointSize(5.0f);
        glBindVertexArray(VAO_masses);
        glDrawArrays(GL_POINTS, 0, massesVertices.size() / 3);

        if (selectedMassIndex != -1)
        {
            shader.setVec3("color", glm::vec3(1.0f, 1.0f, 0.0f));
            glPointSize(10.0f);
            glDrawArrays(GL_POINTS, selectedMassIndex, 1);
        }

        extern int trackedMassIndex;
        extern bool trackingMode;
        if (trackingMode && trackedMassIndex >= 0 && trackedMassIndex < masses.size())
        {
            shader.setVec3("color", glm::vec3(0.0f, 1.0f, 1.0f));
            glPointSize(15.0f);
            glBindVertexArray(VAO_masses);
            glDrawArrays(GL_POINTS, trackedMassIndex, 1);
        }
    }

    glBindVertexArray(0);
}

void Cloth::update(float dt)
{
    simulationTime += dt;

    forceManager.update(dt);

    for (auto &mass : masses)
    {
        if (!mass.fixed)
        {
            glm::vec3 totalForce = forceManager.calculateTotalForce(mass, simulationTime);

            mass.applyForce(totalForce);
        }

        mass.update(dt);
    }

    for (int iter = 0; iter < solverIterations; ++iter)
    {
        for (auto &spring : springs)
        {
            Mass &massA = masses[spring.a];
            Mass &massB = masses[spring.b];

            glm::vec3 delta = massB.position - massA.position;
            float currentLength = glm::length(delta);

            if (currentLength < 0.0001f)
                continue;

            float difference = currentLength - spring.restLength;
            glm::vec3 direction = delta / currentLength;

            if (currentLength > spring.restLength * maxStretchRatio)
            {
                float maxLength = spring.restLength * maxStretchRatio;
                float overStretch = currentLength - maxLength;
                float correction = overStretch * 0.5f;

                if (!massA.fixed)
                    massA.position += direction * correction;
                if (!massB.fixed)
                    massB.position -= direction * correction;
            }
            else
            {
                glm::vec3 correction = direction * difference * 0.5f;
                float correctionFactorScaled = correctionFactor * (spring.stiffness / 100.0f);

                if (!massA.fixed)
                    massA.position += correction * correctionFactorScaled;
                if (!massB.fixed)
                    massB.position -= correction * correctionFactorScaled;
            }
        }

        if (enableCollisions)
            for (auto *obj : collisionObjects)
            {
                for (auto &mass : masses)
                {
                    if (!mass.fixed)
                    {
                        glm::vec3 correction;
                        if (obj->checkCollision(mass.position, correction))
                        {
                            mass.position += correction;

                            float correctionLength = glm::length(correction);
                            if (correctionLength > 0.001f)
                            {
                                glm::vec3 normal = correction / correctionLength;
                            }
                        }
                    }
                }
            }
    }

    for (auto &mass : masses)
    {
        if (mass.position.y < floorY)
        {
            mass.position.y = floorY;
            mass.prevPosition.y = floorY;
        }
    }

    if (enableTensionBreaking)
    {
        checkSpringTension();
    }

    analysis.updateGlobalStats(masses, springs, simulationTime);

    if (trackingMode && trackedMassIndex >= 0 && trackedMassIndex < masses.size())
    {
        analysis.recordMassPointData(trackedMassIndex, masses[trackedMassIndex], springs, simulationTime);
    }

    rebuildGraphicsData();
    rebuildTextureData();

    glBindBuffer(GL_ARRAY_BUFFER, VBO_masses);
    glBufferSubData(GL_ARRAY_BUFFER, 0, massesVertices.size() * sizeof(float), massesVertices.data());

    if (VAO_lines != 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, VBO_lines);
        glBufferSubData(GL_ARRAY_BUFFER, 0, lineVertices.size() * sizeof(float), lineVertices.data());
    }

    if (VAO_texture != 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture);
        glBufferSubData(GL_ARRAY_BUFFER, 0, textureVertices.size() * sizeof(float), textureVertices.data());
    }
}
void Mass::update(float dt)
{
    if (fixed)
    {
        force = glm::vec3(0.0f);
        return;
    }

    glm::vec3 acceleration = force / mass;

    glm::vec3 currentPosition = position;
    float damping = 0.99f;
    position = position + (position - prevPosition) * damping + acceleration * (dt * dt);
    prevPosition = currentPosition;

    force = glm::vec3(0.0f);
}

void Mass::applyForce(const glm::vec3 &force)
{
    this->force += force;
}

int Cloth::pickMassPoint(const Ray &ray)
{
    float closestDistance = std::numeric_limits<float>::max();
    int closestIndex = -1;
    const float PICK_DISTANCE_THRESHOLD = 0.1f;

    for (int i = 0; i < masses.size(); ++i)
    {
        const Mass &mass = masses[i];

        glm::vec3 toMass = mass.position - ray.Origin();
        float projection = glm::dot(toMass, ray.Direction());

        if (projection < 0)
            continue;

        glm::vec3 closestPoint = ray.Origin() + ray.Direction() * projection;
        float distance = glm::length(mass.position - closestPoint);

        if (distance < PICK_DISTANCE_THRESHOLD && distance < closestDistance)
        {
            closestDistance = distance;
            closestIndex = i;
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
        checkTearingAroundPoint(index);
    }
}

void Cloth::checkTearingAroundPoint(int massIndex)
{
    const float tearThreshold = 20.0f;

    bool springsRemoved = false;
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
                it = springs.erase(it);
                springsRemoved = true;
                continue;
            }
        }
        ++it;
    }

    if (springsRemoved)
    {
        rebuildTextureData();
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

void Cloth::cleanupBuffers()
{
    if (VAO_masses != 0)
    {
        glDeleteVertexArrays(1, &VAO_masses);
        glDeleteBuffers(1, &VBO_masses);
        VAO_masses = 0;
        VBO_masses = 0;
    }

    if (VAO_lines != 0)
    {
        glDeleteVertexArrays(1, &VAO_lines);
        glDeleteBuffers(1, &VBO_lines);
        VAO_lines = 0;
        VBO_lines = 0;
    }

    if (VAO_texture != 0)
    {
        glDeleteVertexArrays(1, &VAO_texture);
        glDeleteBuffers(1, &VBO_texture);
        glDeleteBuffers(1, &EBO_texture);
        VAO_texture = 0;
        VBO_texture = 0;
        EBO_texture = 0;
    }
}

void Cloth::resize(float newWidth, float newHeight, int newResX, int newResY)
{
    width = newWidth;
    height = newHeight;
    resX = newResX;
    resY = newResY;

    cleanupBuffers();
    initCloth();
}

void Cloth::cutSpringsWithRay(const Ray &ray, const glm::vec3 &previousMousePos, const glm::mat4 &view,
                              const glm::mat4 &projection, int screenWidth, int screenHeight)
{
    glm::mat4 viewProjection = projection * view;

    std::vector<int> springsToCut;
    springsToCut.reserve(50);

    float cutThresholdPixels = 10.0f;

    auto worldToScreen = [&](const glm::vec3 &worldPos) -> glm::vec3 {
        glm::vec4 clipSpace = viewProjection * glm::vec4(worldPos, 1.0f);

        if (std::abs(clipSpace.w) < 0.0001f)
            return glm::vec3(-9999.0f);

        float invW = 1.0f / clipSpace.w;
        glm::vec3 ndc = glm::vec3(clipSpace) * invW;

        return glm::vec3((ndc.x + 1.0f) * 0.5f * screenWidth, (1.0f - ndc.y) * 0.5f * screenHeight, clipSpace.w);
    };

    float distToPrev = glm::length(previousMousePos - ray.Origin());
    glm::vec3 currentMouseWorldPos = ray.Origin() + ray.Direction() * distToPrev;

    glm::vec3 currentScreen = worldToScreen(currentMouseWorldPos);
    glm::vec3 previousScreen = worldToScreen(previousMousePos);

    if (currentScreen.x < -9000.0f || previousScreen.x < -9000.0f)
    {
        return;
    }

    glm::vec2 screenSegment = glm::vec2(currentScreen.x - previousScreen.x, currentScreen.y - previousScreen.y);
    float segmentLength = glm::length(screenSegment);

    if (segmentLength < 1.0f)
    {
        return;
    }

    glm::vec2 segmentDir = screenSegment / segmentLength;
    glm::vec2 prevScreen2D = glm::vec2(previousScreen.x, previousScreen.y);

    float cutThresholdSq = cutThresholdPixels * cutThresholdPixels;

    float margin = cutThresholdPixels * 1.5f;
    float minX = std::min(previousScreen.x, currentScreen.x) - margin;
    float maxX = std::max(previousScreen.x, currentScreen.x) + margin;
    float minY = std::min(previousScreen.y, currentScreen.y) - margin;
    float maxY = std::max(previousScreen.y, currentScreen.y) + margin;

    for (int i = 0; i < springs.size(); ++i)
    {
        const Mass &massA = masses[springs[i].a];
        const Mass &massB = masses[springs[i].b];

        glm::vec3 springStart = massA.position;
        glm::vec3 springEnd = massB.position;
        glm::vec3 springMid = (springStart + springEnd) * 0.5f;

        glm::vec3 startScreen = worldToScreen(springStart);
        glm::vec3 endScreen = worldToScreen(springEnd);
        glm::vec3 midScreen = worldToScreen(springMid);

        if (startScreen.x < -9000.0f && endScreen.x < -9000.0f && midScreen.x < -9000.0f)
            continue;

        bool shouldCut = false;

        glm::vec3 pointsToCheck[3] = {startScreen, midScreen, endScreen};

        for (int p = 0; p < 3; p++)
        {
            glm::vec3 point = pointsToCheck[p];

            if (point.x < -9000.0f)
                continue;

            if (point.x < minX || point.x > maxX || point.y < minY || point.y > maxY)
                continue;

            glm::vec2 point2D = glm::vec2(point.x, point.y);
            glm::vec2 toPoint = point2D - prevScreen2D;

            float projection = glm::dot(toPoint, segmentDir);
            projection = glm::clamp(projection, 0.0f, segmentLength);

            glm::vec2 closestPoint = prevScreen2D + segmentDir * projection;
            glm::vec2 diff = point2D - closestPoint;

            float distanceSq = diff.x * diff.x + diff.y * diff.y;

            if (distanceSq <= cutThresholdSq)
            {
                shouldCut = true;
                break;
            }
        }

        if (shouldCut)
        {
            springsToCut.push_back(i);
        }
    }

    if (!springsToCut.empty())
    {
        for (int i = springsToCut.size() - 1; i >= 0; --i)
        {
            springs.erase(springs.begin() + springsToCut[i]);
        }

        rebuildGraphicsData();
        rebuildTextureData();
    }
}

void Cloth::checkSpringTension()
{
    if (!enableTensionBreaking)
        return;

    static std::vector<int> tensionCounter;
    if (tensionCounter.size() != springs.size())
    {
        tensionCounter.resize(springs.size(), 0);
    }

    std::vector<int> springsToBreak;
    springsToBreak.reserve(20);

    const int FRAMES_BEFORE_BREAK = 3;

    for (int i = 0; i < springs.size(); ++i)
    {
        const Spring &spring = springs[i];
        const Mass &massA = masses[spring.a];
        const Mass &massB = masses[spring.b];

        float currentLength = glm::length(massB.position - massA.position);

        if (currentLength < 0.0001f)
            continue;
        float stretchRatio = currentLength / spring.restLength;

        if (stretchRatio > tensionBreakThreshold)
        {
            tensionCounter[i]++;
            if (tensionCounter[i] >= FRAMES_BEFORE_BREAK)
            {
                bool shouldBreak = true;
                if ((massA.fixed && !massB.fixed) || (!massA.fixed && massB.fixed))
                {
                    if (stretchRatio < tensionBreakThreshold * 1.5f)
                    {
                        shouldBreak = false;
                    }
                }
                if (shouldBreak)
                {
                    springsToBreak.push_back(i);
                    glm::vec3 breakPos = (massA.position + massB.position) * 0.5f;
                    analysis.recordSpringBreak(i, breakPos, stretchRatio, simulationTime);
                }
            }
        }
        else
        {
            tensionCounter[i] = 0;
        }
    }

    if (!springsToBreak.empty())
    {
        for (int i = springsToBreak.size() - 1; i >= 0; --i)
        {
            int springIdx = springsToBreak[i];
            springs.erase(springs.begin() + springIdx);
            tensionCounter.erase(tensionCounter.begin() + springIdx);
        }
        rebuildGraphicsData();
        rebuildTextureData();
    }
}

AnalysisDisplayData Cloth::getAnalysisDisplayData() const
{
    AnalysisDisplayData data;

    extern int trackedMassIndex;
    extern bool trackingMode;

    data.selectedMassIndex = trackingMode ? trackedMassIndex : -1;
    data.totalSprings = springs.size();
    data.brokenSprings = analysis.getTotalBrokenSprings();
    data.totalEnergy = analysis.getTotalEnergy();
    data.averageSystemTension = analysis.getAverageTension();
    data.maxTension = analysis.getMaxTension();

    if (trackingMode && trackedMassIndex >= 0 && trackedMassIndex < masses.size())
    {
        const Mass &mass = masses[trackedMassIndex];
        data.position = mass.position;
        data.velocity = analysis.calculateVelocity(mass);
        data.speed = glm::length(data.velocity);
        data.acceleration = mass.acceleration;
        data.kineticEnergy = analysis.calculateKineticEnergy(mass);
        data.averageTension = analysis.calculateAverageSpringTension(trackedMassIndex, masses, springs);

        data.connectedSprings = 0;
        for (const auto &spring : springs)
        {
            if (spring.a == trackedMassIndex || spring.b == trackedMassIndex)
                data.connectedSprings++;
        }
    }

    const auto &history = analysis.getHistoryData();
    data.timeHistory.reserve(history.size());
    data.velocityHistory.reserve(history.size());
    data.energyHistory.reserve(history.size());
    data.tensionHistory.reserve(history.size());

    for (const auto &point : history)
    {
        data.timeHistory.push_back(point.time);
        data.velocityHistory.push_back(glm::length(point.velocity));
        data.energyHistory.push_back(point.kineticEnergy);
        data.tensionHistory.push_back(point.averageSpringTension);
    }

    return data;
}

void Cloth::freeCloth()
{
    for (auto &mass : masses)
        mass.fixed = false;
}

void Cloth::addCollisionObject(Object *obj)
{
    if (obj != nullptr)
    {
        collisionObjects.push_back(obj);
    }
}

void Cloth::removeCollisionObject(Object *obj)
{
    collisionObjects.erase(std::remove(collisionObjects.begin(), collisionObjects.end(), obj), collisionObjects.end());
}

void Cloth::clearCollisionObjects()
{
    collisionObjects.clear();
}

void Cloth::setOrientation(ClothOrientation orientation)
{
    if (currentOrientation != orientation)
    {
        currentOrientation = orientation;
        selectedMassIndex = -1;
        initCloth();
    }
}

void Cloth::setPhysicalProperties(float mass, float structStiff, float structDamp, float shearStiff, float shearDamp,
                                  float bendStiff, float bendDamp)
{
    defaultMass = mass;
    defaultStructuralStiffness = structStiff;
    defaultStructuralDamping = structDamp;
    defaultShearStiffness = shearStiff;
    defaultShearDamping = shearDamp;
    defaultBendingStiffness = bendStiff;
    defaultBendingDamping = bendDamp;
}

void Cloth::setSolverParameters(int iterations, float correction, float maxStretch)
{
    solverIterations = iterations;
    correctionFactor = correction;
    maxStretchRatio = maxStretch;
}

const ForceManager &Cloth::getForceManager() const
{
    return forceManager;
}

const float Cloth::getClothWidth() const
{
    return width;
}

const float Cloth::getClothHeight() const
{
    return height;
}

const std::vector<Mass> &Cloth::getMasses() const
{
    return masses;
}

const std::vector<Spring> &Cloth::getSprings() const
{
    return springs;
}

Mass &Cloth::getMass(int index)
{
    return masses[index];
}

ForceManager &Cloth::getForceManager()
{
    return forceManager;
}
void Cloth::changeMassesVisible()
{
    massVisible = !massVisible;
}
void Cloth::changeSpringsVisible()
{
    springVisible = !springVisible;
}

void Cloth::changeTextureVisible()
{
    textureVisible = !textureVisible;
}

void Cloth::setTensionBreaking(float threshold)
{
    tensionBreakThreshold = threshold;
}

void Cloth::setTensionBreakThreshold(float threshold)
{
    tensionBreakThreshold = threshold;
}

void Cloth::setCutThreshold(float threshold)
{
    cutThresholdPixels = threshold;
}

void Cloth::setEnableTensionBreaking(bool enabled)
{
    enableTensionBreaking = enabled;
}

float Cloth::getTensionBreaking() const
{
    return tensionBreakThreshold;
}

float Cloth::getTensionBreakThreshold() const
{
    return tensionBreakThreshold;
}

float Cloth::getCutThreshold() const
{
    return cutThresholdPixels;
}

bool Cloth::getEnableTensionBreaking() const
{
    return enableTensionBreaking;
}

ClothAnalysis &Cloth::getAnalysis()
{
    return analysis;
}

const ClothAnalysis &Cloth::getAnalysis() const
{
    return analysis;
}

void Cloth::setEnableCollisions(bool enabled)
{
    enableCollisions = enabled;
}
bool Cloth::getEnableCollisions() const
{
    return enableCollisions;
}

Cloth::ClothOrientation Cloth::getOrientation() const
{
    return currentOrientation;
}