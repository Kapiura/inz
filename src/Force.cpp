#include "Force.hpp"
#include "Cloth.hpp"

glm::vec3 GravityForce::calculate(const Mass& mass, float time) const
{
    if (!enabled) return glm::vec3(0.0f);
    return glm::vec3(0.0f, gravity * mass.mass, 0.0f);
}

glm::vec3 RepulsionForce::calculate(const Mass& mass, float time) const
{
    if (!enabled) return glm::vec3(0.0f);
    
    glm::vec3 toMass = mass.position - center;
    float distance = glm::length(toMass);
    
    if (distance > radius || distance < 0.001f)
        return glm::vec3(0.0f);
    
    float falloff = 1.0f - (distance / radius);
    return glm::normalize(toMass) * strength * falloff * falloff;
}

glm::vec3 AttractionForce::calculate(const Mass& mass, float time) const
{
    if (!enabled) return glm::vec3(0.0f);
    
    glm::vec3 toCenter = center - mass.position;
    float distance = glm::length(toCenter);
    
    if (distance < 0.001f)
        return glm::vec3(0.0f);
    
    return glm::normalize(toCenter) * strength;
}

glm::vec3 ForceManager::calculateTotalForce(const Mass& mass, float time) const
{
    glm::vec3 total(0.0f);
    for (const auto& force : forces)
        total += force->calculate(mass, time);
    return total;
}