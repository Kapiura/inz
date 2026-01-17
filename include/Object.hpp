#pragma once

#include <glm/glm.hpp>
#include <vector>

class Mass;

class Object
{
public:
    virtual ~Object() = default;
    virtual bool checkCollision(const glm::vec3& point, glm::vec3& correction) const = 0;
    virtual glm::vec3 getPosition() const = 0;
    virtual void setPosition(const glm::vec3& pos) = 0;
    virtual void render() const = 0;
};

class Sphere : public Object
{
public:
    Sphere(const glm::vec3& center, float radius)
        : center(center), radius(radius) {}
    
    bool checkCollision(const glm::vec3& point, glm::vec3& correction) const override;
    
    glm::vec3 getPosition() const override { return center; }
    void setPosition(const glm::vec3& pos) override { center = pos; }
    void render() const override {}
    
    float getRadius() const { return radius; }
    void setRadius(float r) { radius = r; }
    
private:
    glm::vec3 center;
    float radius;
};