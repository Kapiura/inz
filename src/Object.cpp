#include "Object.hpp"
#include "Shader.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <iostream>
#include <array>
#include <algorithm>

bool Sphere::checkCollision(const glm::vec3& point, glm::vec3& correction) const
{
    glm::vec3 diff = point - center;
    
    float distSq = glm::dot(diff, diff);
    float radiusSq = radius * radius;
    
    if (distSq < radiusSq)
    {
        if (distSq < 0.00001f)
        {
            correction = glm::vec3(0.0f, radius, 0.0f);
            return true;
        }
        
        float dist = std::sqrt(distSq);
        
        glm::vec3 direction = diff / dist;
        
        float penetration = radius - dist;
        
        correction = direction * penetration;
        
        return true;
    }
    
    return false;
}

Cube::Cube(const glm::vec3& center, const glm::vec3& size, const char* texturePath)
    : center(center), size(size), rotation(0.0f), VAO(0), VBO(0), textureID(0)
{
    setupCube();
    loadTexture(texturePath);
}

Cube::~Cube()
{
    if (VAO != 0)
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
    if (textureID != 0)
    {
        glDeleteTextures(1, &textureID);
    }
}

void Cube::setupCube()
{
    glm::vec3 halfSize = size * 0.5f;
    glm::vec3 min = center - halfSize;
    glm::vec3 max = center + halfSize;
    
    float vertices[] = 
    {
        // positions          // normals           // texcoords

        // BACK (-Z)
        min.x, min.y, min.z,  0,0,-1,   0.25f,1.0f,
        max.x, min.y, min.z,  0,0,-1,   0.5f,1.0f,
        max.x, max.y, min.z,  0,0,-1,   0.5f,2.0f/3.0f,
        max.x, max.y, min.z,  0,0,-1,   0.5f,2.0f/3.0f,
        min.x, max.y, min.z,  0,0,-1,   0.25f,2.0f/3.0f,
        min.x, min.y, min.z,  0,0,-1,   0.25f,1.0f,

        // FRONT (+Z)
        min.x, min.y, max.z,  0,0,1,    0.25f,1.0f,
        max.x, min.y, max.z,  0,0,1,    0.5f,1.0f,
        max.x, max.y, max.z,  0,0,1,    0.5f,2.0f/3.0f,
        max.x, max.y, max.z,  0,0,1,    0.5f,2.0f/3.0f,
        min.x, max.y, max.z,  0,0,1,    0.25f,2.0f/3.0f,
        min.x, min.y, max.z,  0,0,1,    0.25f,1.0f,

        // LEFT (-X)
        min.x, max.y, max.z, -1,0,0,    0.25f,2.0f/3.0f,
        min.x, max.y, min.z, -1,0,0,    0.25f,1.0f/3.0f,
        min.x, min.y, min.z, -1,0,0,    0.0f, 1.0f/3.0f,
        min.x, min.y, min.z, -1,0,0,    0.0f, 1.0f/3.0f,
        min.x, min.y, max.z, -1,0,0,    0.0f, 2.0f/3.0f,
        min.x, max.y, max.z, -1,0,0,    0.25f,2.0f/3.0f,

        // RIGHT (+X)
        max.x, max.y, max.z,  1,0,0,    0.5f, 2.0f/3.0f,
        max.x, max.y, min.z,  1,0,0,    0.5f, 1.0f/3.0f,
        max.x, min.y, min.z,  1,0,0,    0.75f,1.0f/3.0f,
        max.x, min.y, min.z,  1,0,0,    0.75f,1.0f/3.0f,
        max.x, min.y, max.z,  1,0,0,    0.75f,2.0f/3.0f,
        max.x, max.y, max.z,  1,0,0,    0.5f, 2.0f/3.0f,

        // BOTTOM (-Y)
        min.x, min.y, min.z,  0,-1,0,   0.75f,2.0f/3.0f,
        max.x, min.y, min.z,  0,-1,0,   0.75f,1.0f/3.0f,
        max.x, min.y, max.z,  0,-1,0,   1.0f, 1.0f/3.0f,
        max.x, min.y, max.z,  0,-1,0,   1.0f, 1.0f/3.0f,
        min.x, min.y, max.z,  0,-1,0,   1.0f, 2.0f/3.0f,
        min.x, min.y, min.z,  0,-1,0,   0.75f,2.0f/3.0f,

        // TOP (+Y)
        min.x, max.y, min.z,  0,1,0,    0.5f, 1.0f/3.0f,
        max.x, max.y, min.z,  0,1,0,    0.5f, 2.0f/3.0f,
        max.x, max.y, max.z,  0,1,0,    0.25f,2.0f/3.0f,
        max.x, max.y, max.z,  0,1,0,    0.25f,2.0f/3.0f,
        min.x, max.y, max.z,  0,1,0,    0.25f,1.0f/3.0f,
        min.x, max.y, min.z,  0,1,0,    0.5f, 1.0f/3.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Cube::loadTexture(const char* path)
{
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

    if (data)
    {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);
}

void Cube::render(Shader& shader) const
{
    glm::mat4 model = glm::mat4(1.0f);
    
    model = glm::translate(model, center);
    model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::translate(model, -center);
    
    shader.setMat4("model", model);
    shader.setInt("useTexture", 1);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

bool Cube::checkCollision(const glm::vec3& point, glm::vec3& correction) const
{
    glm::mat4 rotationMatrix = glm::mat4(1.0f);
    rotationMatrix = glm::rotate(rotationMatrix, -rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    rotationMatrix = glm::rotate(rotationMatrix, -rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    rotationMatrix = glm::rotate(rotationMatrix, -rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    
    glm::vec3 localPoint = glm::vec3(rotationMatrix * glm::vec4(point - center, 1.0f));
    
    glm::vec3 halfSize = size * 0.5f;
    const float edgeMargin = 0.05f;
    
    glm::vec3 closest;
    closest.x = glm::clamp(localPoint.x, -halfSize.x, halfSize.x);
    closest.y = glm::clamp(localPoint.y, -halfSize.y, halfSize.y);
    closest.z = glm::clamp(localPoint.z, -halfSize.z, halfSize.z);
    
    glm::vec3 delta = localPoint - closest;
    float distanceSquared = glm::dot(delta, delta);
    
    const float collisionRadius = 0.15f;
    
    bool inside = (localPoint.x >= -halfSize.x && localPoint.x <= halfSize.x &&
                   localPoint.y >= -halfSize.y && localPoint.y <= halfSize.y &&
                   localPoint.z >= -halfSize.z && localPoint.z <= halfSize.z);
    
    bool nearSurface = distanceSquared < (collisionRadius * collisionRadius);
    
    if (inside || nearSurface)
    {
        glm::vec3 localCorrection(0.0f);
        
        if (inside)
        {
            glm::vec3 distances;
            distances.x = halfSize.x - std::abs(localPoint.x);
            distances.y = halfSize.y - std::abs(localPoint.y);
            distances.z = halfSize.z - std::abs(localPoint.z);
            
            float minDist = std::min({distances.x, distances.y, distances.z});
            
            const float pushoutEpsilon = 0.02f;
            
            if (std::abs(distances.x - minDist) < 0.001f)
            {
                localCorrection.x = (localPoint.x > 0) ? (distances.x + pushoutEpsilon) 
                                                       : -(distances.x + pushoutEpsilon);
            }
            else if (std::abs(distances.y - minDist) < 0.001f)
            {
                localCorrection.y = (localPoint.y > 0) ? (distances.y + pushoutEpsilon) 
                                                       : -(distances.y + pushoutEpsilon);
            }
            else
            {
                localCorrection.z = (localPoint.z > 0) ? (distances.z + pushoutEpsilon) 
                                                       : -(distances.z + pushoutEpsilon);
            }
        }
        else
        {
            float distance = std::sqrt(distanceSquared);
            
            if (distance > 0.0001f)
            {
                glm::vec3 direction = delta / distance;
                float penetration = collisionRadius - distance;
                localCorrection = direction * (penetration + 0.02f);
            }
            else
            {
                localCorrection = glm::vec3(0.0f, collisionRadius + 0.02f, 0.0f);
            }
        }
        
        glm::mat4 invRotationMatrix = glm::mat4(1.0f);
        invRotationMatrix = glm::rotate(invRotationMatrix, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        invRotationMatrix = glm::rotate(invRotationMatrix, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        invRotationMatrix = glm::rotate(invRotationMatrix, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        
        correction = glm::vec3(invRotationMatrix * glm::vec4(localCorrection, 0.0f));
        
        return true;
    }
    
    return false;
}