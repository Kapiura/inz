#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

#include "Shader.hpp"
#include "Texture.hpp"

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
    glm::vec2 uv;

    AABB getAABB() const;

    Mass(const glm::vec3 &pos, float m, bool fix = false, const glm::vec2 &texCoord = glm::vec2(0.0f))
        : position(pos), prevPosition(pos), velocity(0.0f), acceleration(0.0f), force(0.0f), mass(m), fixed(fix),
          uv(texCoord)
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

struct Triangle
{
    int a, b, c;

    Triangle(int a, int b, int c) : a(a), b(b), c(c)
    {
    }

    bool sharesVertexWith(const Triangle &other) const
    {
        return (a == other.a || a == other.b || a == other.c || b == other.a || b == other.b || b == other.c ||
                c == other.a || c == other.b || c == other.c);
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

    void rebuildTrianglesFromSprings();
    void removeIsolatedMasses();

    void setTexture(Texture *tex);

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

    const float getClothWidth() const
    {
        return width;
    }
    const float getClothHeight() const
    {
        return height;
    }

  private:
    int resX, resY;
    float width, height;

    void rebuildGraphicsData();
    void applyConsts();
    bool springIntersectsSegment(const Spring &spring, const glm::vec3 &segmentStart, const glm::vec3 &segmentEnd,
                                 glm::vec3 &intersectionPoint);
    void initCloth();

    bool areSpringMidpointsConnected(int springA, int springB) const;
    glm::vec3 getTriangleNormal(const Triangle &tri) const;
    bool isPointInTriangle(const glm::vec3 &p, const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c) const;

    std::vector<Mass> masses;
    std::vector<Spring> springs;
    std::vector<Triangle> triangles;

    bool massVisible = false;
    bool springVisible = false;

    std::vector<float> massesVertices;
    std::vector<float> lineVertices;

    std::vector<unsigned int> indices;

    unsigned int VAO_cloth = 0, VBO_cloth = 0, VBO_uv = 0, EBO_cloth = 0;

    unsigned int VAO_masses = 0, VBO_masses = 0;
    unsigned int VAO_lines = 0, VBO_lines = 0;

    const float gravity = -9.81f;
    float floorY = 0.0f;

    int selectedMassIndex = -1;
    glm::vec3 lastMouseWorldPos;

    Texture *texture = nullptr;
};