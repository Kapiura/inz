#include "AABB.hpp"
#include "Ray.hpp"
#include <iostream>

AABB::AABB(const glm::vec3 &min, const glm::vec3 &max) : m_min(min), m_max(max)
{
}

bool AABB::intersect(const Ray &ray, float &t) const
{
    glm::vec3 invDir = 1.0f / ray.Direction();

    float t1 = (m_min.x - ray.Origin().x) * invDir.x;
    float t2 = (m_max.x - ray.Origin().x) * invDir.x;
    float t3 = (m_min.y - ray.Origin().y) * invDir.y;
    float t4 = (m_max.y - ray.Origin().y) * invDir.y;
    float t5 = (m_min.z - ray.Origin().z) * invDir.z;
    float t6 = (m_max.z - ray.Origin().z) * invDir.z;

    float tmin = glm::max(glm::max(glm::min(t1, t2), glm::min(t3, t4)), glm::min(t5, t6));
    float tmax = glm::min(glm::min(glm::max(t1, t2), glm::max(t3, t4)), glm::max(t5, t6));

    if (tmax < 0)
    {
        t = tmax;
        return false;
    }

    if (tmin > tmax)
    {
        t = tmax;
        return false;
    }

    t = tmin;
    return true;
}