#pragma once

#include <glm/glm.hpp>

class Ray;

class AABB
{
  public:
    AABB(const glm::vec3 &min, const glm::vec3 &max);

    glm::vec3 getMin() const
    {
        return m_min;
    }
    glm::vec3 getMax() const
    {
        return m_max;
    }
    glm::vec3 getCenter() const
    {
        return (m_min + m_max) * 0.5f;
    }

    bool intersect(const Ray &ray, float &t) const;

  private:
    glm::vec3 m_min;
    glm::vec3 m_max;
};