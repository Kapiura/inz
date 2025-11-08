#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

#include "Shader.hpp"

class AABB;
class Ray;

struct Mass
{
    glm::vec3 position;
    glm::vec3 prevPosition;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    glm::vec3 force;
    float mass;
    bool fixed;

    AABB getAABB() const;

    Mass(const glm::vec3 &pos, float m, bool fix = false)
        : position(pos), prevPosition(pos), velocity(0.0f), acceleration(0.0f), force(0.0f), mass(m), fixed(fix)
    {
    }

    void update(float dt);
    void applyForce(glm::vec3 &&force);
};

struct Spring
{
    int a, b;
    float restLength;
    float stiffness;
    float damping;
    glm::vec3 midpoint;

    Spring(int a, int b, float rest, const glm::vec3 &mp, float k = 50.0f, float d = 0.01f)
        : a(a), b(b), restLength(rest), stiffness(k), damping(d), midpoint(mp)
    {
    }
};

class Cloth
{
  public:
    Cloth(float width, float height, int resX, int resY, float floorY);
    void draw(Shader &shader);
    void update(float dt);
    void satisfy();
    void reset();

    // methods to pick masses
    int pickMassPoint(const Ray &ray);
    void setMassPosition(int index, const glm::vec3 &position);
    void releaseMassPoint(int index);

    void checkTearingAroundPoint(int massIndex);
    void cutSpringsWithRay(const Ray &ray, const glm::vec3 &previousMousePos);

    // mass getters
    Mass &getMass(int index)
    {
        return masses[index];
    }
    const std::vector<Mass> &getMasses() const
    {
        return masses;
    }
    void changeMassesVisible()
    {
        massVisible = !massVisible;
    }
    void changeSpringsVisible()
    {
        springVisible = !springVisible;
    }

  private:
    int resX, resY;
    float width, height;

    void rebuildGraphicsData();
    void applyConsts();
    bool springIntersectsSegment(const Spring &spring, const glm::vec3 &segmentStart, const glm::vec3 &segmentEnd,
                                 glm::vec3 &intersectionPoint);
    void initCloth();

    std::vector<Mass> masses;
    std::vector<Spring> springs;

    bool massVisible = true;
    bool springVisible = true;

    std::vector<float> massesVertices;
    std::vector<float> lineVertices;

    std::vector<float> texCoords;
    std::vector<unsigned int> indices;
    unsigned int VAO_cloth = 0, VBO_cloth = 0, EBO_cloth = 0;

    unsigned int VAO_masses = 0, VBO_masses = 0;
    unsigned int VAO_lines = 0, VBO_lines = 0;

    const float gravity = -9.81f;
    float floorY = 0.0f;

    int selectedMassIndex = -1;
    glm::vec3 lastMouseWorldPos;
};
