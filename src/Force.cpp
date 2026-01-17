#include "Force.hpp"
#include "Cloth.hpp"

glm::vec3 GravityForce::calculate(const Mass& mass, float time) const
{
    if (!enabled) return glm::vec3(0.0f);
    return glm::vec3(0.0f, gravity * mass.mass, 0.0f);
}

glm::vec3 ForceManager::calculateTotalForce(const Mass& mass, float time) const
{
    glm::vec3 total(0.0f);
    for (const auto& force : forces)
        total += force->calculate(mass, time);
    return total;
}