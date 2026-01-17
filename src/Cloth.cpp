#include "AABB.hpp"
#include "AnalysisData.hpp"
#include "Cloth.hpp"
#include "Ray.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <queue>
#include <stb_image.h>
#include <map>
#include <unordered_map>
#include <tuple>

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
    // Resize z zerami - szybsze niż push_back
    const size_t massVertCount = masses.size() * 3;
    const size_t springVertCount = springs.size() * 6;
    
    if (massesVertices.size() != massVertCount)
        massesVertices.resize(massVertCount);
    
    if (lineVertices.size() != springVertCount)
        lineVertices.resize(springVertCount);
    
    // Masses - optymalizacja z bezpośrednim dostępem
    for (size_t i = 0; i < masses.size(); i++)
    {
        const size_t idx = i * 3;
        const auto& pos = masses[i].position;
        massesVertices[idx] = pos.x;
        massesVertices[idx + 1] = pos.y;
        massesVertices[idx + 2] = pos.z;
    }

    // Springs - optymalizacja
    for (size_t i = 0; i < springs.size(); i++)
    {
        const size_t idx = i * 6;
        const auto& spring = springs[i];
        const auto& posA = masses[spring.a].position;
        const auto& posB = masses[spring.b].position;
        
        lineVertices[idx] = posA.x;
        lineVertices[idx + 1] = posA.y;
        lineVertices[idx + 2] = posA.z;
        lineVertices[idx + 3] = posB.x;
        lineVertices[idx + 4] = posB.y;
        lineVertices[idx + 5] = posB.z;
    }
}

void Cloth::calculateNormals()
{
    // Reset normali
    for (auto &mass : masses)
        mass.normal = glm::vec3(0.0f);
    
    // Oblicz normalne dla trójkątów
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
            
            // Pierwszy trójkąt
            glm::vec3 edge1 = masses[idx1].position - masses[idx0].position;
            glm::vec3 edge2 = masses[idx2].position - masses[idx0].position;
            glm::vec3 normal1 = glm::cross(edge1, edge2);  // Bez normalizacji tutaj
            
            masses[idx0].normal += normal1;
            masses[idx1].normal += normal1;
            masses[idx2].normal += normal1;
            
            // Drugi trójkąt
            edge1 = masses[idx3].position - masses[idx1].position;
            edge2 = masses[idx2].position - masses[idx1].position;
            glm::vec3 normal2 = glm::cross(edge1, edge2);
            
            masses[idx1].normal += normal2;
            masses[idx3].normal += normal2;
            masses[idx2].normal += normal2;
        }
    }
    
    // Normalizuj na końcu
    for (auto &mass : masses)
    {
        float lengthSq = glm::dot(mass.normal, mass.normal);
        if (lengthSq > 0.001f)
            mass.normal *= (1.0f / sqrt(lengthSq));  // Szybsza normalizacja
        else
            mass.normal = glm::vec3(0.0f, 0.0f, 1.0f);
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
    simulationTime += dt;

    // =========================
    // FAZA 1: INTEGRACJA (Verlet)
    // =========================
    for (Mass &m : masses)
    {
        if (m.fixed) continue;

        glm::vec3 totalForce = forceManager.calculateTotalForce(m, simulationTime);
        m.applyForce(std::move(totalForce));
        m.update(dt);
    }

    // =========================
    // FAZA 2: SOLVER SPRĘŻYN (TYLKO HARD CONSTRAINT)
    // =========================
// SOLVER SPRĘŻYN – HARD LIMIT (CLAMP DO maxLength)
// sprężyna NIGDY nie przekroczy maxStretchRatio * restLength

constexpr float MIN_LEN_SQ = 1e-6f;

for (int iter = 0; iter < solverIterations; ++iter)
{
    for (const Spring &spring : springs)
    {
        Mass &a = masses[spring.a];
        Mass &b = masses[spring.b];

        glm::vec3 delta = b.position - a.position;
        float lenSq = glm::dot(delta, delta);
        if (lenSq < MIN_LEN_SQ) continue;

        float restLen = spring.restLength;
        float maxLen  = restLen * maxStretchRatio;
        float maxLenSq = maxLen * maxLen;

        if (lenSq <= maxLenSq)
            continue;

        float len    = std::sqrt(lenSq);
        float invLen = 1.0f / len;
        glm::vec3 dir = delta * invLen;

        float wA = a.fixed ? 0.0f : 1.0f;
        float wB = b.fixed ? 0.0f : 1.0f;
        float wSum = wA + wB;
        if (wSum == 0.0f) continue;

        float excess = len - maxLen;

        glm::vec3 correction = dir * (excess / wSum);

        a.position += correction * wA;
        b.position -= correction * wB;
    }
}


    // =========================
    // FAZA 3: KOLIZJE (co 2 klatki)
    // =========================
    static int collisionFrameCounter = 0;
    if (++collisionFrameCounter == 2)
    {
        collisionFrameCounter = 0;
        handleObjectCollisions();
        handleSelfCollision();
    }

    // =========================
    // FAZA 4: PODŁOGA
    // =========================
    for (Mass &m : masses)
    {
        if (m.position.y >= floorY) continue;

        m.position.y = floorY;

        glm::vec3 v = m.position - m.prevPosition;
        v.y = -v.y * 0.3f;
        m.prevPosition = m.position - v;
    }

    // =========================
    // FAZA 5: TENSION BREAKING
    // =========================
    if (enableTensionBreaking)
        checkSpringTension();

    // =========================
    // FAZA 6: ANALIZA
    // =========================
    if (trackingMode &&
        trackedMassIndex >= 0 &&
        trackedMassIndex < static_cast<int>(masses.size()))
    {
        analysis.updateGlobalStats(masses, springs, simulationTime);
        analysis.recordMassPointData(
            trackedMassIndex,
            masses[trackedMassIndex],
            springs,
            simulationTime
        );
    }

    // =========================
    // FAZA 7: REBUILD CPU
    // =========================
    rebuildGraphicsData();

    static int textureCounter = 0;
    bool rebuildTexture = (++textureCounter == 3);
    if (rebuildTexture)
    {
        textureCounter = 0;
        rebuildTextureData();
    }

    // =========================
    // FAZA 8: GPU UPLOAD
    // =========================
    glBindBuffer(GL_ARRAY_BUFFER, VBO_masses);
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        massesVertices.size() * sizeof(float),
        massesVertices.data()
    );

    if (VAO_lines)
    {
        glBindBuffer(GL_ARRAY_BUFFER, VBO_lines);
        glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            lineVertices.size() * sizeof(float),
            lineVertices.data()
        );
    }

    if (rebuildTexture && VAO_texture)
    {
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture);
        glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            textureVertices.size() * sizeof(float),
            textureVertices.data()
        );
    }
}



void Mass::update(float dt)
{
    glm::vec3 velocity = position - prevPosition;
    
    const float damping = 0.98f;  // 2% strata energii per frame
    velocity *= damping;
    
    prevPosition = position;
    position += velocity + acceleration * (dt * dt);
    
    acceleration = glm::vec3(0.0f);
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

void Cloth::resize(float newWidth, float newHeight, int newResX, int newResY)
{
    width = newWidth;
    height = newHeight;
    resX = newResX;
    resY = newResY;
    
    initCloth();
    
    std::cout << "Cloth resized to " << width << "x" << height 
              << " with resolution " << resX << "x" << resY << std::endl;
}

void Cloth::cutSpringsWithRay(const Ray &ray, const glm::vec3 &previousMousePos,
                               const glm::mat4 &view, const glm::mat4 &projection,
                               int screenWidth, int screenHeight)
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
        
        return glm::vec3(
            (ndc.x + 1.0f) * 0.5f * screenWidth,
            (1.0f - ndc.y) * 0.5f * screenHeight,
            clipSpace.w
        );
    };
    
    float distToPrev = glm::length(previousMousePos - ray.Origin());
    glm::vec3 currentMouseWorldPos = ray.Origin() + ray.Direction() * distToPrev;
    
    glm::vec3 currentScreen = worldToScreen(currentMouseWorldPos);
    glm::vec3 previousScreen = worldToScreen(previousMousePos);
    
    if (currentScreen.x < -9000.0f || previousScreen.x < -9000.0f)
    {
        return;
    }
    
    glm::vec2 screenSegment = glm::vec2(currentScreen.x - previousScreen.x, 
                                         currentScreen.y - previousScreen.y);
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
        
        glm::vec3 pointsToCheck[3] = { startScreen, midScreen, endScreen };
        
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
        
        std::cout << "Cut " << springsToCut.size() << " springs\n";
    }
}

void Cloth::checkSpringTension()
{
    if (!enableTensionBreaking)
        return;
    
    static std::vector<int> tensionCounter;
    if (tensionCounter.size() != springs.size())
        tensionCounter.resize(springs.size(), 0);
    
    std::vector<int> springsToBreak;
    springsToBreak.reserve(50);
    
    const int FRAMES_BEFORE_BREAK = 3;
    const float thresholdSq = tensionBreakThreshold * tensionBreakThreshold;
    
    for (size_t i = 0; i < springs.size(); ++i)
    {
        const Spring& spring = springs[i];
        const Mass& massA = masses[spring.a];
        const Mass& massB = masses[spring.b];
        
        glm::vec3 diff = massB.position - massA.position;
        float currentLengthSq = glm::dot(diff, diff);
        
        if (currentLengthSq < 0.0001f)
            continue;
        
        float restLengthSq = spring.restLength * spring.restLength;
        float stretchRatioSq = currentLengthSq / restLengthSq;
        
        if (stretchRatioSq > thresholdSq)
        {
            tensionCounter[i]++;
            
            if (tensionCounter[i] >= FRAMES_BEFORE_BREAK)
            {
                // Zwiększona tolerancja dla przypiętych punktów
                bool shouldBreak = true;
                if ((massA.fixed && !massB.fixed) || (!massA.fixed && massB.fixed))
                {
                    if (stretchRatioSq < thresholdSq * 3.0f)  // 1.73^2 ≈ 3
                        shouldBreak = false;
                }
                
                if (shouldBreak)
                {
                    springsToBreak.push_back(i);
                    float stretchRatio = std::sqrt(stretchRatioSq);
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
    
    // Usuń sprężyny od tyłu
    if (!springsToBreak.empty())
    {
        for (int i = (int)springsToBreak.size() - 1; i >= 0; --i)
        {
            int springIdx = springsToBreak[i];
            springs.erase(springs.begin() + springIdx);
            tensionCounter.erase(tensionCounter.begin() + springIdx);
        }
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
        const Mass& mass = masses[trackedMassIndex];
        data.position = mass.position;
        data.velocity = analysis.calculateVelocity(mass);
        data.speed = glm::length(data.velocity);
        data.acceleration = mass.acceleration;
        data.kineticEnergy = analysis.calculateKineticEnergy(mass);
        data.averageTension = analysis.calculateAverageSpringTension(
            trackedMassIndex, masses, springs);
        
        data.connectedSprings = 0;
        for (const auto& spring : springs)
        {
            if (spring.a == trackedMassIndex || spring.b == trackedMassIndex)
                data.connectedSprings++;
        }
    }
    
    const auto& history = analysis.getHistoryData();
    data.timeHistory.reserve(history.size());
    data.velocityHistory.reserve(history.size());
    data.energyHistory.reserve(history.size());
    data.tensionHistory.reserve(history.size());
    
    for (const auto& point : history)
    {
        data.timeHistory.push_back(point.time);
        data.velocityHistory.push_back(glm::length(point.velocity));
        data.energyHistory.push_back(point.kineticEnergy);
        data.tensionHistory.push_back(point.averageSpringTension);
    }
    
    return data;
}

void Cloth::handleSelfCollision()
{
    if (!enableSelfCollision)
        return;
    
    // Sprawdzaj co 3 klatki dla wydajności
    static int frameSkip = 0;
    if (++frameSkip < 3)
        return;
    frameSkip = 0;
    
    // Wyłącz dla dużych siatek
    if (masses.size() > 2000)
    {
        enableSelfCollision = false;
        std::cout << "Self-collision disabled: too many masses\n";
        return;
    }
    
    const float radius = massCollisionRadius;
    const float diameter = radius * 2.0f;
    const float diameterSq = diameter * diameter;
    
    // Buduj mapę bezpośrednich sąsiadów (tylko ci nie kolidują)
    std::vector<std::set<int>> neighbors(masses.size());
    for (const auto& spring : springs)
    {
        neighbors[spring.a].insert(spring.b);
        neighbors[spring.b].insert(spring.a);
    }
    
    // Sprawdź każdą parę mas
    int collisions = 0;
    const int MAX_COLLISIONS = 50;
    
    for (int i = 0; i < masses.size() && collisions < MAX_COLLISIONS; i++)
    {
        if (masses[i].fixed)
            continue;
        
        for (int j = i + 1; j < masses.size() && collisions < MAX_COLLISIONS; j++)
        {
            if (masses[j].fixed)
                continue;
            
            // Pomiń bezpośrednich sąsiadów
            if (neighbors[i].count(j) > 0)
                continue;
            
            // Sprawdź odległość między środkami kulek
            glm::vec3 diff = masses[j].position - masses[i].position;
            float distSq = glm::dot(diff, diff);
            
            // Kule się przecinają jeśli odległość < suma promieni (diameter)
            if (distSq < diameterSq && distSq > 0.00001f)
            {
                float dist = std::sqrt(distSq);
                glm::vec3 normal = diff / dist;
                
                // Penetracja = jak głęboko kule weszły w siebie
                float penetration = diameter - dist;
                
                // Wypchaj każdą kulę o połowę penetracji
                glm::vec3 correction = normal * (penetration * 0.5f);
                
                masses[i].position -= correction;
                masses[j].position += correction;
                
                collisions++;
            }
        }
    }
}

void Cloth::addCollisionObject(std::unique_ptr<Object> obj)
{
    collisionObjects.push_back(std::move(obj));
}

void Cloth::clearCollisionObjects()
{
    collisionObjects.clear();
}

std::vector<Object*> Cloth::getCollisionObjects()
{
    std::vector<Object*> result;
    for (auto& obj : collisionObjects)
        result.push_back(obj.get());
    return result;
}

void Cloth::handleObjectCollisions()
{
    for (auto& mass : masses)
    {
        if (mass.fixed)
            continue;
            
        for (const auto& obj : collisionObjects)
        {
            glm::vec3 correction;
            if (obj->checkCollision(mass.position, correction))
            {
                // KROK 1: Popraw pozycję - wypchaj na powierzchnię
                mass.position += correction;
                
                // KROK 2: Zeruj prędkość w kierunku normalnym (zapobiega drganiom)
                glm::vec3 velocity = mass.position - mass.prevPosition;
                glm::vec3 normal = glm::normalize(correction);
                
                float normalVelocity = glm::dot(velocity, normal);
                
                // Jeśli porusza się w kierunku sfery, odbij z tłumieniem
                if (normalVelocity < 0.0f)
                {
                    // Usuń składową normalną i odwróć ją z tłumieniem
                    velocity -= normal * normalVelocity * 1.5f;
                }
                
                // BARDZO WAŻNE: Zastosuj silne tłumienie aby zapobiec drganiom
                mass.prevPosition = mass.position - velocity * 0.2f;
            }
        }
    }
}

void Cloth::setPinMode(PinMode mode)
{
    if (currentPinMode == mode)
        return;
        
    currentPinMode = mode;
    
    int pinnedCount = 0;
    
    // Odepnij wszystkie punkty
    for (auto& mass : masses)
        mass.fixed = false;
    
    // Przypnij zgodnie z wybranym trybem
    switch(mode)
    {
        case PinMode::TOP_EDGE:
            for (int x = 0; x < resX; x++)
            {
                int gridIdx = 0 * resX + x;
                if (gridIdx < massIndexMap.size())
                {
                    int massIdx = massIndexMap[gridIdx];
                    if (massIdx >= 0 && massIdx < masses.size())
                    {
                        masses[massIdx].fixed = true;
                        pinnedCount++;
                    }
                }
            }
            std::cout << "Pin mode: Full top edge (" << pinnedCount << " points)\n";
            break;
            
        case PinMode::TWO_CORNERS:
            {
                int gridIdx = 0 * resX + 0;
                if (gridIdx < massIndexMap.size())
                {
                    int massIdx = massIndexMap[gridIdx];
                    if (massIdx >= 0 && massIdx < masses.size())
                    {
                        masses[massIdx].fixed = true;
                        pinnedCount++;
                    }
                }
            }
            {
                int gridIdx = 0 * resX + (resX - 1);
                if (gridIdx < massIndexMap.size())
                {
                    int massIdx = massIndexMap[gridIdx];
                    if (massIdx >= 0 && massIdx < masses.size())
                    {
                        masses[massIdx].fixed = true;
                        pinnedCount++;
                    }
                }
            }
            std::cout << "Pin mode: Two corners (" << pinnedCount << " points)\n";
            break;
            
        case PinMode::CENTER_PIN:
            // POPRAWKA: Środek całej siatki (środek Y i środek X)
            {
                int centerY = resY / 2;
                int centerX = resX / 2;
                int gridIdx = centerY * resX + centerX;
                
                if (gridIdx < massIndexMap.size())
                {
                    int massIdx = massIndexMap[gridIdx];
                    if (massIdx >= 0 && massIdx < masses.size())
                    {
                        masses[massIdx].fixed = true;
                        pinnedCount++;
                        std::cout << "Pinned center at grid (" << centerX << ", " << centerY 
                                  << ") = mass #" << massIdx << "\n";
                    }
                }
            }
            std::cout << "Pin mode: Center point (" << pinnedCount << " point)\n";
            break;
    }
}

void Cloth::removeCollisionObject(size_t index)
{
    if (index < collisionObjects.size())
    {
        collisionObjects.erase(collisionObjects.begin() + index);
        std::cout << "Removed collision object #" << index << "\n";
    }
}