#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

#include "Shader.hpp"
#include "Force.hpp"

class AABB;
class Ray;

struct Mass
{
    glm::vec3 position;
    glm::vec3 prevPosition;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    glm::vec3 force;
    glm::vec3 normal;
    float mass;
    bool fixed;
    glm::vec2 texCoord;

    AABB getAABB() const;

    Mass(const glm::vec3 &pos, float m, bool fix = false, const glm::vec2 &tc = glm::vec2(0.0f))
        : position(pos), prevPosition(pos), velocity(0.0f), acceleration(0.0f), 
          force(0.0f), normal(0.0f, 0.0f, 1.0f), mass(m), fixed(fix), texCoord(tc)
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

    int pickMassPoint(const Ray &ray);
    void setMassPosition(int index, const glm::vec3 &position);
    void releaseMassPoint(int index);

    void checkTearingAroundPoint(int massIndex);
    void cutSpringsWithRay(const Ray &ray, const glm::vec3 &previousMousePos);
    void calculateNormals();

    void removeIsolatedMasses();
    void resize(float newWidth, float newHeight, int newResX, int newResY);

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
    void changeTextureVisible()
    {
        textureVisible = !textureVisible;
    }

    const float getClothWidth() const
    {
        return width;
    }
    const float getClothHeight() const
    {
        return height;
    }
    
    ForceManager& getForceManager() { return forceManager; }
    const ForceManager& getForceManager() const { return forceManager; }

  private:
    int resX, resY;
    float width, height;

    void rebuildGraphicsData();
    void rebuildTextureData();
    void applyConsts();
    bool springIntersectsSegment(const Spring &spring, const glm::vec3 &segmentStart, const glm::vec3 &segmentEnd,
                                 glm::vec3 &intersectionPoint);
    void initCloth();

    bool areSpringMidpointsConnected(int springA, int springB) const;

    std::vector<Mass> masses;
    std::vector<Spring> springs;
    std::vector<int> massIndexMap;

    bool massVisible = false;
    bool springVisible = false;
    bool textureVisible = true;

    std::vector<float> massesVertices;
    std::vector<float> lineVertices;
    std::vector<float> textureVertices;
    std::vector<unsigned int> textureIndices;

    unsigned int VAO_masses = 0, VBO_masses = 0;
    unsigned int VAO_lines = 0, VBO_lines = 0;
    unsigned int VAO_texture = 0, VBO_texture = 0, EBO_texture = 0;
    unsigned int textureID = 0;

    float floorY = 0.0f;
    
    ForceManager forceManager;
    float simulationTime = 0.0f;

    int selectedMassIndex = -1;
    glm::vec3 lastMouseWorldPos;
};
