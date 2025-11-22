#include "Cloth.hpp"
#include "AABB.hpp"
#include "Ray.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <queue>
#include <stb_image.h>

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

    masses.reserve(resX * resY);
    springs.reserve((resX - 1) * resY + resX * (resY - 1) + (resX - 1) * (resY - 1));
    massIndexMap.resize(resX * resY);

    const float massValue = 0.5f;

    float structuralStiff = 100.0f;
    float structuralDamping = 1.5f;
    float shearStiff = 200.0f;
    float shearDamping = 1.0f;
    float bendingStiff = 100.0f;
    float bendingDamping = 0.8f;

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
            
            int gridIdx = y * resX + x;
            massIndexMap[gridIdx] = gridIdx;
        }
    }

    // Structural horizontal springs
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

    // Structural vertical springs
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

    // Shear diagonal springs
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

    // Bending horizontal springs
    for(int y = 0; y < resY; y++)  
    {
        for(int x = 0; x < resX - 2; x++)
        {
            int idx1 = y * resX + x;
            int idx2 = y * resX + (x + 2);
            float length = glm::distance(masses[idx1].position, masses[idx2].position);
            glm::vec3 mp = midpoint(masses[idx1].position, masses[idx2].position);
            springs.emplace_back(idx1, idx2, length, mp, bendingStiff, bendingDamping);
        }
    }

    // Bending vertical springs
    for(int y = 0; y < resY - 2; y++)
    {
        for(int x = 0; x < resX; x++)
        {
            int idx1 = y * resX + x;
            int idx2 = (y + 2) * resX + x;
            float length = glm::distance(masses[idx1].position, masses[idx2].position);
            glm::vec3 mp = midpoint(masses[idx1].position, masses[idx2].position);
            springs.emplace_back(idx1, idx2, length, mp, bendingStiff, bendingDamping);
        }
    }

    // Load texture
    if (textureID == 0)
    {
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        int width, height, nrChannels;
        unsigned char *data = stbi_load("../img/textures/cloth.png", &width, &height, &nrChannels, 0);
        
        if (data)
        {
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            std::cout << "Loaded texture: " << width << "x" << height << std::endl;
        }
        else
        {
            std::cout << "Failed to load texture, using checkerboard pattern\n";
            unsigned char checkerboard[64];
            for(int i = 0; i < 64; i++)
                checkerboard[i] = ((i/8 + (i%8)) % 2) ? 255 : 0;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 8, 8, 0, GL_RED, GL_UNSIGNED_BYTE, checkerboard);
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
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, textureIndices.size() * sizeof(unsigned int), textureIndices.data(), GL_DYNAMIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
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
    std::cout << "Reset cloth\n";
}

Cloth::Cloth(float width, float height, int resX, int resY, float floorY)
    : width(width), height(height), resX(resX), resY(resY), floorY(floorY)
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
            
            if (gridIdx0 >= massIndexMap.size() || gridIdx1 >= massIndexMap.size() ||
                gridIdx2 >= massIndexMap.size() || gridIdx3 >= massIndexMap.size())
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

            if (gridIdx0 >= massIndexMap.size() || gridIdx1 >= massIndexMap.size() ||
                gridIdx2 >= massIndexMap.size() || gridIdx3 >= massIndexMap.size())
                continue;

            int idx0 = massIndexMap[gridIdx0];
            int idx1 = massIndexMap[gridIdx1];
            int idx2 = massIndexMap[gridIdx2];
            int idx3 = massIndexMap[gridIdx3];

            if (idx0 < 0 || idx1 < 0 || idx2 < 0 || idx3 < 0)
                continue;

            int edgeCount1 = 0;
            if (hasSpring(idx0, idx1)) edgeCount1++;
            if (hasSpring(idx1, idx2)) edgeCount1++;
            if (hasSpring(idx0, idx2)) edgeCount1++;
            
            if (edgeCount1 >= 2)
            {
                textureIndices.push_back(idx0);
                textureIndices.push_back(idx1);
                textureIndices.push_back(idx2);
            }

            int edgeCount2 = 0;
            if (hasSpring(idx1, idx3)) edgeCount2++;
            if (hasSpring(idx3, idx2)) edgeCount2++;
            if (hasSpring(idx1, idx2)) edgeCount2++;
            
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
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, textureIndices.size() * sizeof(unsigned int), textureIndices.data(), GL_DYNAMIC_DRAW);
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
    }

    glBindVertexArray(0);
}

void Cloth::removeIsolatedMasses()
{
    if (masses.empty())
        return;

    std::vector<int> springCount(masses.size(), 0);
    
    for (const auto &spring : springs)
    {
        if (spring.a < masses.size() && spring.b < masses.size())
        {
            springCount[spring.a]++;
            springCount[spring.b]++;
        }
    }

    std::vector<bool> toRemove(masses.size(), false);
    int removeCount = 0;
    
    for (int i = 0; i < masses.size(); i++)
    {
        if (springCount[i] == 0 && !masses[i].fixed)
        {
            toRemove[i] = true;
            removeCount++;
        }
    }

    std::vector<std::vector<int>> adjacency(masses.size());
    for (const auto &spring : springs)
    {
        if (spring.a < masses.size() && spring.b < masses.size())
        {
            adjacency[spring.a].push_back(spring.b);
            adjacency[spring.b].push_back(spring.a);
        }
    }

    std::vector<bool> connectedToFixed(masses.size(), false);
    std::queue<int> queue;
    
    for (int i = 0; i < masses.size(); i++)
    {
        if (masses[i].fixed)
        {
            connectedToFixed[i] = true;
            queue.push(i);
        }
    }

    while (!queue.empty())
    {
        int current = queue.front();
        queue.pop();

        for (int neighbor : adjacency[current])
        {
            if (neighbor < masses.size() && !connectedToFixed[neighbor])
            {
                connectedToFixed[neighbor] = true;
                queue.push(neighbor);
            }
        }
    }

    for (int i = 0; i < masses.size(); i++)
    {
        if (!connectedToFixed[i] && !masses[i].fixed && !toRemove[i])
        {
            toRemove[i] = true;
            removeCount++;
        }
    }

    if (removeCount == 0)
        return;

    std::cout << "Removing " << removeCount << " isolated/floating masses\n";

    std::vector<int> newIndex(masses.size(), -1);
    int currentNew = 0;
    
    for (int i = 0; i < masses.size(); i++)
    {
        if (!toRemove[i])
        {
            newIndex[i] = currentNew++;
        }
    }

    std::vector<Mass> newMasses;
    newMasses.reserve(masses.size() - removeCount);
    
    for (int i = 0; i < masses.size(); i++)
    {
        if (!toRemove[i])
        {
            newMasses.push_back(masses[i]);
        }
    }

    std::vector<Spring> newSprings;
    newSprings.reserve(springs.size());
    
    for (auto &spring : springs)
    {
        if (spring.a < masses.size() && spring.b < masses.size() && 
            !toRemove[spring.a] && !toRemove[spring.b])
        {
            spring.a = newIndex[spring.a];
            spring.b = newIndex[spring.b];
            newSprings.push_back(spring);
        }
    }

    masses = std::move(newMasses);
    springs = std::move(newSprings);

    for (int gridIdx = 0; gridIdx < massIndexMap.size(); gridIdx++)
    {
        int oldMassIdx = massIndexMap[gridIdx];
        if (oldMassIdx >= 0 && oldMassIdx < toRemove.size() && !toRemove[oldMassIdx])
        {
            massIndexMap[gridIdx] = newIndex[oldMassIdx];
        }
        else
        {
            massIndexMap[gridIdx] = -1;
        }
    }

    if (selectedMassIndex != -1)
    {
        if (selectedMassIndex >= toRemove.size() || toRemove[selectedMassIndex])
        {
            selectedMassIndex = -1;
        }
        else
        {
            selectedMassIndex = newIndex[selectedMassIndex];
        }
    }
}

void Cloth::update(float dt)
{
    for (auto &mass : masses)
    {
        if (!mass.fixed)
            mass.applyForce(glm::vec3(0.0f, gravity, 0.0f));
        mass.update(dt);
    }
    for (int iter = 0; iter < 5; ++iter)
    {
        for (auto &spring : springs)
        {
            Mass &massA = masses[spring.a];
            Mass &massB = masses[spring.b];

            glm::vec3 delta = massB.position - massA.position;
            float currentLength = glm::length(delta);

            if (currentLength > 0.0001f)
            {
                float maxStretchRatio = 1.2f;
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
                    glm::vec3 direction = delta / currentLength;
                    glm::vec3 correction = direction * difference * 0.5f;

                    float correctionFactor = 0.15f * (spring.stiffness / 100.0f);

                    if (!massA.fixed)
                        massA.position += correction * correctionFactor;
                    if (!massB.fixed)
                        massB.position -= correction * correctionFactor;
                }
            }
        }
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

    static int frameCounter = 0;
    if (++frameCounter >= 30)
    {
        // removeIsolatedMasses();
        frameCounter = 0;
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
    glm::vec3 velocity = position - prevPosition;
    
    const float damping = 0.99f;
    velocity *= damping;
    
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

        rebuildGraphicsData();
        rebuildTextureData();

        std::cout << "Cut " << springsToCut.size() << " springs\n";
    }
}
