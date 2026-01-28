#pragma once

#include <glm/glm.hpp>

class Ray
{
  public:
    using time_t = float;

    enum class HitType
    {
        Hit,
        Miss
    };

    Ray(const glm::vec3 &origin, const glm::vec3 &direction);

    // Get ray start point
    glm::vec3 Origin() const
    {
        return m_origin;
    }
    // Get ray direction
    glm::vec3 Direction() const
    {
        return m_direction;
    }
    // Get point along rat at distance t
    glm::vec3 At(Ray::time_t t) const;

  private:
    // Ray data
    glm::vec3 m_origin;
    glm::vec3 m_direction;
};