#include "Cloth.hpp"
#include "AABB.hpp"
#include "Ray.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <queue>

glm::vec3 midpoint(const glm::vec3 &a, const glm::vec3 &b)
{
    return (a + b) * 0.5f;
}

glm::vec3 Cloth::getTriangleNormal(const Triangle &tri) const
{
    const glm::vec3 &p0 = masses[tri.a].position;
    const glm::vec3 &p1 = masses[tri.b].position;
    const glm::vec3 &p2 = masses[tri.c].position;

    glm::vec3 edge1 = p1 - p0;
    glm::vec3 edge2 = p2 - p0;

    glm::vec3 normal = glm::cross(edge1, edge2);
    float len = glm::length(normal);

    if (len > 0.0001f)
        return normal / len;

    return glm::vec3(0.0f, 0.0f, 1.0f);
}

bool Cloth::isPointInTriangle(const glm::vec3 &p, const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c) const
{
    glm::vec3 v0 = c - a;
    glm::vec3 v1 = b - a;
    glm::vec3 v2 = p - a;

    float dot00 = glm::dot(v0, v0);
    float dot01 = glm::dot(v0, v1);
    float dot02 = glm::dot(v0, v2);
    float dot11 = glm::dot(v1, v1);
    float dot12 = glm::dot(v1, v2);

    float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

    return (u >= -0.001f) && (v >= -0.001f) && (u + v <= 1.001f);
}

bool Cloth::areSpringMidpointsConnected(int springA, int springB) const
{
    const Spring &sA = springs[springA];
    const Spring &sB = springs[springB];

    return (sA.a == sB.a || sA.a == sB.b || sA.b == sB.a || sA.b == sB.b);
}

bool Cloth::springMidpointTriangleCollision(const glm::vec3 &midpoint, int springIndex, const Triangle &tri,
                                            glm::vec3 &normal, float &distance)
{
    const glm::vec3 &p0 = masses[tri.a].position;
    const glm::vec3 &p1 = masses[tri.b].position;
    const glm::vec3 &p2 = masses[tri.c].position;

    normal = getTriangleNormal(tri);

    float dist = glm::dot(midpoint - p0, normal);
    distance = std::abs(dist);

    if (distance > collisionThickness)
        return false;

    const Spring &spring = springs[springIndex];
    if (tri.a == spring.a || tri.a == spring.b || tri.b == spring.a || tri.b == spring.b || tri.c == spring.a ||
        tri.c == spring.b)
        return false;

    glm::vec3 projectedPoint = midpoint - normal * dist;

    if (!isPointInTriangle(projectedPoint, p0, p1, p2))
        return false;

    return true;
}

void Cloth::handleSelfCollision()
{
    if (!selfCollisionEnabled)
        return;

    const float responseStrength = 0.4f;
    const float massResponseRatio = 0.6f;

    for (int i = 0; i < springs.size(); ++i)
    {
        Spring &spring = springs[i];

        if (masses[spring.a].fixed && masses[spring.b].fixed)
            continue;

        glm::vec3 currentMidpoint = spring.getCurrentMidpoint(masses);

        for (int t = 0; t < triangles.size(); ++t)
        {
            const Triangle &tri = triangles[t];

            if (tri.a == spring.a || tri.a == spring.b || tri.b == spring.a || tri.b == spring.b || tri.c == spring.a ||
                tri.c == spring.b)
                continue;

            bool isAdjacent = false;
            for (const auto &checkSpring : springs)
            {
                bool belongsToTriangle = (checkSpring.a == tri.a && checkSpring.b == tri.b) ||
                                         (checkSpring.a == tri.b && checkSpring.b == tri.a) ||
                                         (checkSpring.a == tri.b && checkSpring.b == tri.c) ||
                                         (checkSpring.a == tri.c && checkSpring.b == tri.b) ||
                                         (checkSpring.a == tri.c && checkSpring.b == tri.a) ||
                                         (checkSpring.a == tri.a && checkSpring.b == tri.c);

                if (belongsToTriangle)
                {
                    if (checkSpring.a == spring.a || checkSpring.a == spring.b || checkSpring.b == spring.a ||
                        checkSpring.b == spring.b)
                    {
                        isAdjacent = true;
                        break;
                    }
                }
            }

            if (isAdjacent)
                continue;

            glm::vec3 normal;
            float distance;

            if (springMidpointTriangleCollision(currentMidpoint, i, tri, normal, distance))
            {
                const glm::vec3 &p0 = masses[tri.a].position;
                float side = glm::dot(currentMidpoint - p0, normal);

                glm::vec3 correction = normal * (collisionThickness - distance) * responseStrength;

                if (side < 0)
                    correction = -correction;

                if (!masses[spring.a].fixed)
                    masses[spring.a].position += correction * massResponseRatio;
                if (!masses[spring.b].fixed)
                    masses[spring.b].position += correction * massResponseRatio;

                float triCorrection = (1.0f - massResponseRatio) / 3.0f;
                if (!masses[tri.a].fixed)
                    masses[tri.a].position -= correction * triCorrection;
                if (!masses[tri.b].fixed)
                    masses[tri.b].position -= correction * triCorrection;
                if (!masses[tri.c].fixed)
                    masses[tri.c].position -= correction * triCorrection;
            }
        }
    }
}

void Cloth::initCloth()
{
    masses.clear();
    springs.clear();
    triangles.clear();

    masses.reserve(resX * resY);
    springs.reserve((resX - 1) * resY + resX * (resY - 1) + (resX - 1) * (resY - 1));

    const float massValue = 1.0f;

    float structuralStiff = 80.0f;
    float structuralDamping = 0.5f;
    float shearStiff = 60.0f;
    float shearDamping = 0.3f;

    for (int y = 0; y < resY; y++)
    {
        for (int x = 0; x < resX; x++)
        {
            float xpos = (x / float(resX - 1)) * width - width / 2.0f;
            float ypos = 5.0f - (y / float(resY - 1)) * height;
            float zpos = 0.0f;

            float u = x / float(resX - 1);
            float v = y / float(resY - 1);

            bool isFixed = (y == 0);
            masses.emplace_back(glm::vec3(xpos, ypos, zpos), massValue, isFixed, glm::vec2(u, v));
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

    for (int y = 0; y < resY - 1; y++)
    {
        for (int x = 0; x < resX - 1; x++)
        {
            int topLeft = y * resX + x;
            int topRight = topLeft + 1;
            int bottomLeft = (y + 1) * resX + x;
            int bottomRight = bottomLeft + 1;

            triangles.push_back({topLeft, bottomLeft, topRight});
            triangles.push_back({topRight, bottomLeft, bottomRight});
        }
    }

    rebuildGraphicsData();

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

    if (VAO_cloth == 0)
    {
        glGenVertexArrays(1, &VAO_cloth);
        glGenBuffers(1, &VBO_cloth);
        glGenBuffers(1, &EBO_cloth);

        glBindVertexArray(VAO_cloth);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_cloth);
        glBufferData(GL_ARRAY_BUFFER, massesVertices.size() * sizeof(float), massesVertices.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_cloth);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
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
    std::cout << "Reset cloth\n";
}

Cloth::Cloth(float width, float height, int resX, int resY, float floorY)
    : width(width), height(height), resX(resX), resY(resY), floorY(floorY)
{
    VAO_masses = 0, VBO_masses = 0;
    VAO_lines = 0, VBO_lines = 0;
    VAO_cloth = 0, VBO_cloth = 0, EBO_cloth = 0;

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

    indices.clear();
    for (const auto &tri : triangles)
    {
        indices.push_back(tri.a);
        indices.push_back(tri.b);
        indices.push_back(tri.c);
    }

    std::vector<float> texCoords;
    texCoords.reserve(masses.size() * 2);
    for (const auto &mass : masses)
    {
        texCoords.push_back(mass.uv.x);
        texCoords.push_back(mass.uv.y);
    }

    if (VAO_cloth == 0)
    {
        glGenVertexArrays(1, &VAO_cloth);
        glGenBuffers(1, &VBO_cloth);
        glGenBuffers(1, &VBO_uv);
        glGenBuffers(1, &EBO_cloth);
    }

    glBindVertexArray(VAO_cloth);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_cloth);
    glBufferData(GL_ARRAY_BUFFER, massesVertices.size() * sizeof(float), massesVertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_uv);
    glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(float), texCoords.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_cloth);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_DYNAMIC_DRAW);
}

void Cloth::draw(Shader &shader)
{
    if (springVisible)
    {
        shader.setBool("useTexture", false);
        shader.setVec3("color", glm::vec3(0.0f, 0.0f, 1.0f));
        glBindVertexArray(VAO_lines);
        glDrawArrays(GL_LINES, 0, lineVertices.size() / 3);
    }

    if (massVisible)
    {
        shader.setBool("useTexture", false);
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
    }

    if (texture != nullptr)
    {
        shader.setBool("useTexture", true);
        texture->bind(0);
        shader.setInt("clothTexture", 0);
    }
    else
    {
        shader.setBool("useTexture", false);
        shader.setVec3("color", glm::vec3(0.8f, 0.8f, 0.8f));
    }

    glBindVertexArray(VAO_cloth);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}

void Cloth::setTexture(Texture *tex)
{
    texture = tex;
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

        handleSelfCollision();
    }

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
    glm::vec3 planeNormal = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 planePoint = glm::vec3(0.0f, 2.5f, 0.0f);

    float denom = glm::dot(planeNormal, ray.Direction());
    glm::vec3 currentMouseWorldPos;

    if (std::abs(denom) > 0.0001f)
    {
        float t = glm::dot(planePoint - ray.Origin(), planeNormal) / denom;
        if (t >= 0)
        {
            currentMouseWorldPos = ray.Origin() + ray.Direction() * t;
        }
        else
        {
            float distance = glm::length(previousMousePos - ray.Origin());
            currentMouseWorldPos = ray.Origin() + ray.Direction() * distance;
        }
    }
    else
    {
        float distance = glm::length(previousMousePos - ray.Origin());
        currentMouseWorldPos = ray.Origin() + ray.Direction() * distance;
    }

    glm::vec3 mouseSegment = currentMouseWorldPos - previousMousePos;
    float mouseSegmentLength = glm::length(mouseSegment);

    if (mouseSegmentLength < 0.001f)
    {
        return;
    }

    std::vector<int> springsToCut;

    float cutThreshold = 0.08f;

    for (int i = 0; i < springs.size(); ++i)
    {
        const Mass &massA = masses[springs[i].a];
        const Mass &massB = masses[springs[i].b];

        glm::vec3 springStart = massA.position;
        glm::vec3 springEnd = massB.position;

        glm::vec3 d1 = currentMouseWorldPos - previousMousePos;
        glm::vec3 d2 = springEnd - springStart;
        glm::vec3 r = previousMousePos - springStart;

        float a = glm::dot(d1, d1);
        float b = glm::dot(d1, d2);
        float c = glm::dot(d2, d2);
        float d = glm::dot(d1, r);
        float e = glm::dot(d2, r);

        float denom = a * c - b * b;

        float s, t;

        if (denom < 0.00001f)
        {
            s = 0.0f;
            t = (b > c ? d / b : e / c);
        }
        else
        {
            s = (b * e - c * d) / denom;
            t = (a * e - b * d) / denom;
        }

        s = glm::clamp(s, 0.0f, 1.0f);
        t = glm::clamp(t, 0.0f, 1.0f);

        glm::vec3 closestOnMouse = previousMousePos + d1 * s;
        glm::vec3 closestOnSpring = springStart + d2 * t;

        float dist = glm::length(closestOnMouse - closestOnSpring);

        if (dist <= cutThreshold)
        {
            springsToCut.push_back(i);
        }
    }

    if (!springsToCut.empty())
    {
        for (int i = springsToCut.size() - 1; i >= 0; --i)
        {
            int index = springsToCut[i];
            springs.erase(springs.begin() + index);
        }

        rebuildTrianglesFromSprings();
        rebuildGraphicsData();

        std::cout << "Cut " << springsToCut.size() << " springs\n";
    }
}

void Cloth::rebuildTrianglesFromSprings()
{
    triangles.clear();

    auto springExists = [this](int a, int b) -> bool {
        for (const auto &s : springs)
        {
            if ((s.a == a && s.b == b) || (s.a == b && s.b == a))
                return true;
        }
        return false;
    };

    for (int y = 0; y < resY - 1; y++)
    {
        for (int x = 0; x < resX - 1; x++)
        {
            int topLeft = y * resX + x;
            int topRight = topLeft + 1;
            int bottomLeft = (y + 1) * resX + x;
            int bottomRight = bottomLeft + 1;

            bool hasEdge1 = springExists(topLeft, bottomLeft);
            bool hasEdge2 = springExists(bottomLeft, topRight);
            bool hasEdge3 = springExists(topRight, topLeft);

            if (hasEdge1 && hasEdge2 && hasEdge3)
            {
                triangles.push_back({topLeft, bottomLeft, topRight});
            }

            bool hasEdge4 = springExists(topRight, bottomLeft);
            bool hasEdge5 = springExists(bottomLeft, bottomRight);
            bool hasEdge6 = springExists(bottomRight, topRight);

            if (hasEdge4 && hasEdge5 && hasEdge6)
            {
                triangles.push_back({topRight, bottomLeft, bottomRight});
            }
        }
    }
}

void Cloth::removeIsolatedMasses()
{
}

void Cloth::satisfy()
{
}