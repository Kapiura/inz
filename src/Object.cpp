#include "Object.hpp"
#include <glm/glm.hpp>

bool Sphere::checkCollision(const glm::vec3& point, glm::vec3& correction) const
{
    // Wektor od środka kuli do punktu
    glm::vec3 diff = point - center;
    
    // Odległość od środka kuli
    float distSq = glm::dot(diff, diff);
    float radiusSq = radius * radius;
    
    // Czy punkt jest WEWNĄTRZ kuli?
    if (distSq < radiusSq)
    {
        // Unikaj dzielenia przez zero
        if (distSq < 0.00001f)
        {
            // Punkt dokładnie w środku - wypchaj w górę
            correction = glm::vec3(0.0f, radius, 0.0f);
            return true;
        }
        
        float dist = std::sqrt(distSq);
        
        // Kierunek od środka kuli do punktu (na zewnątrz)
        glm::vec3 direction = diff / dist;
        
        // O ile punkt jest za głęboko w kuli
        float penetration = radius - dist;
        
        // Wypchaj punkt NA powierzchnię kuli
        correction = direction * penetration;
        
        return true;
    }
    
    return false;
}