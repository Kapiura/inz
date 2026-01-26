#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <glad/glad.h>

class Mass;
class Shader;

class Object
{
public:
    virtual ~Object() = default;
    virtual bool checkCollision(const glm::vec3& point, glm::vec3& correction) const = 0;
    virtual glm::vec3 getPosition() const = 0;
    virtual void setPosition(const glm::vec3& pos) = 0;
    virtual void render(Shader& shader) const = 0;
};

class Sphere : public Object
{
public:
    Sphere(const glm::vec3& center, float radius)
        : center(center), radius(radius) {}
    
    bool checkCollision(const glm::vec3& point, glm::vec3& correction) const override;
    
    glm::vec3 getPosition() const override { return center; }
    void setPosition(const glm::vec3& pos) override { center = pos; }
    void render(Shader& shader) const override {}
    
    float getRadius() const { return radius; }
    void setRadius(float r) { radius = r; }
    
private:
    glm::vec3 center;
    float radius;
};

struct FaceCenters {
        glm::vec3 front;   // +Z
        glm::vec3 back;    // -Z
        glm::vec3 right;   // +X
        glm::vec3 left;    // -X
        glm::vec3 top;     // +Y
        glm::vec3 bottom;  // -Y
    };

class Cube : public Object
{
public:
    Cube(const glm::vec3& center, const glm::vec3& size, const char* texturePath);
    ~Cube();

    
    
    bool checkCollision(const glm::vec3& point, glm::vec3& correction) const override;
    
    glm::vec3 getPosition() const override { return center; }
    void setPosition(const glm::vec3& pos) override { center = pos; }
    void render(Shader& shader) const override;
    
    glm::vec3 getSize() const { return size; }
    void setSize(const glm::vec3& s) { size = s; }
    
    glm::vec3 getRotation() const { return rotation; }
    void setRotation(const glm::vec3& rot) { rotation = rot; }


    glm::vec3 getMinBounds() const { return center - size * 0.5f; }
    glm::vec3 getMaxBounds() const { return center + size * 0.5f; }
    
    std::array<glm::vec3, 8> getWorldVertices() const; 
    bool containsPoint(const glm::vec3& point) const;

    FaceCenters getFaceCenters() const;
    
private:
    void setupCube();
    void loadTexture(const char* path);
    
    glm::vec3 center;
    glm::vec3 size;
    glm::vec3 rotation;
    
    GLuint VAO, VBO;
    GLuint textureID;
};