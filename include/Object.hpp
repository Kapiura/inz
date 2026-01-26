#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class Mass;
class Shader;

// Base Object class
class Object
{
  public:
    virtual ~Object() = default;

    // Collision
    virtual bool checkCollision(const glm::vec3 &point, glm::vec3 &correction) const = 0;

    // Position
    virtual glm::vec3 getPosition() const = 0;
    virtual void setPosition(const glm::vec3 &pos) = 0;

    // Rendering
    virtual void render(Shader &shader) const = 0;
};

// Center points of all cube faces
struct FaceCenters
{
    glm::vec3 front;  // +Z
    glm::vec3 back;   // -Z
    glm::vec3 right;  // +X
    glm::vec3 left;   // -X
    glm::vec3 top;    // +Y
    glm::vec3 bottom; // -Y
};

// Cube Object
class Cube : public Object
{
  public:
    Cube(const glm::vec3 &center, const glm::vec3 &size, const char *texturePath);
    ~Cube();

    // Rendering
    void render(Shader &shader) const override;

    // Collision
    bool checkCollision(const glm::vec3 &point, glm::vec3 &correction) const override;
    bool containsPoint(const glm::vec3 &point) const;

    // Position
    void setPosition(const glm::vec3 &pos) override
    {
        center = pos;
    }
    glm::vec3 getPosition() const override
    {
        return center;
    }

    // Cube data
    glm::vec3 getSize() const
    {
        return size;
    }
    void setSize(const glm::vec3 &s)
    {
        size = s;
    }
    glm::vec3 getRotation() const
    {
        return rotation;
    }
    void setRotation(const glm::vec3 &rot)
    {
        rotation = rot;
    }

    // Math
    glm::vec3 getMinBounds() const
    {
        return center - size * 0.5f;
    }
    glm::vec3 getMaxBounds() const
    {
        return center + size * 0.5f;
    }
    std::array<glm::vec3, 8> getWorldVertices() const;
    FaceCenters getFaceCenters() const;

  private:
    // Cube data
    glm::vec3 center;
    glm::vec3 size;
    glm::vec3 rotation;

    // Rendering data
    GLuint VAO, VBO;
    GLuint textureID;

    // Rendering methods
    void setupCube();
    void loadTexture(const char *path);
};