#include "AABB.hpp"
#include "Ray.hpp"
#include <iostream>

AABB::AABB(const glm::vec3 &min, const glm::vec3 &max) : m_min(min), m_max(max)
{
}

bool AABB::intersect(const Ray &ray, float &t) const
{
    glm::vec3 invDir = 1.0f / ray.Direction();
    glm::vec3 t1 = (m_min - ray.Origin()) * invDir;
    glm::vec3 t2 = (m_max - ray.Origin()) * invDir;

    glm::vec3 tmin = glm::min(t1, t2);
    glm::vec3 tmax = glm::max(t1, t2);

    float tmin_val = glm::max(glm::max(tmin.x, tmin.y), tmin.z);
    float tmax_val = glm::min(glm::min(tmax.x, tmax.y), tmax.z);

    if (tmax_val >= 0 && tmin_val <= tmax_val)
    {
        std::cout << "AABB hit! t=" << tmin_val << std::endl;
    }

    if (tmax_val < 0 || tmin_val > tmax_val)
        return false;

    t = tmin_val;
    return true;
}