#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

#include "AnalysisData.hpp"
#include "Force.hpp"
#include "Object.hpp"
#include "Shader.hpp"

extern bool trackingMode;
extern int trackedMassIndex;

class AABB;
class Ray;

struct Mass
{
    // Position and Verlet data
    glm::vec3 position;
    glm::vec3 prevPosition;
    glm::vec3 acceleration;
    glm::vec3 force;
    glm::vec3 normal;

    // Physical data
    float mass;
    bool fixed;

    // Texture data
    glm::vec2 texCoord;

    Mass(const glm::vec3 &pos, float m, bool fix = false, const glm::vec2 &tc = glm::vec2(0.0f))
        : position(pos), prevPosition(pos), acceleration(0.0f), force(0.0f), normal(0.0f, 0.0f, 1.0f), mass(m),
          fixed(fix), texCoord(tc)
    {
    }

    // Update functions
    void update(float dt);
    void applyForce(const glm::vec3 &force);

    // Get AABB ray collision
    AABB getAABB() const;
};

struct Spring
{
    // Two points indexes
    int a, b;
    // Physical data
    float restLength;
    float stiffness;
    float damping;

    // Helper
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

    enum class ClothOrientation
    {
        VERTICAL,
        HORIZONTAL
    };

    // Main functions
    void reset();
    void update(float dt);
    void freeCloth();

    // Physic solver
    void satisfy();
    void calculateNormals();
    void checkSpringTension();

    // Rendering
    void draw(Shader &shader);

    // Cloth setup
    void resize(float newWidth, float newHeight, int newResX, int newResY);
    void setOrientation(ClothOrientation orientation);

    // Collision
    void addCollisionObject(Object *obj);
    void removeCollisionObject(Object *obj);
    void clearCollisionObjects();
    void setEnableCollisions(bool enabled);
    bool getEnableCollisions() const;

    // User interaction
    void setMassPosition(int index, const glm::vec3 &position);
    void releaseMassPoint(int index);
    void cutSpringsWithRay(const Ray &ray, const glm::vec3 &previousMousePos, const glm::mat4 &view,
                           const glm::mat4 &projection, int screenWidth, int screenHeight);
    void checkTearingAroundPoint(int massIndex);
    int pickMassPoint(const Ray &ray);

    // Physical data
    void setSolverParameters(int iterations, float correction, float maxStretch);
    void setPhysicalProperties(float mass, float structStiff, float structDamp, float shearStiff, float shearDamp,
                               float bendStiff, float bendDamp);
    void setCutThreshold(float threshold);
    ;
    void setTensionBreaking(float threshold);
    void setTensionBreakThreshold(float threshold);
    void setEnableTensionBreaking(bool enabled);

    // Visual
    void changeMassesVisible();
    void changeSpringsVisible();
    void changeTextureVisible();

    // Data access
    const float getClothWidth() const;
    const float getClothHeight() const;
    const ClothAnalysis &getAnalysis() const;
    const ForceManager &getForceManager() const;
    ClothAnalysis &getAnalysis();
    ForceManager &getForceManager();
    Mass &getMass(int index);
    const std::vector<Mass> &getMasses() const;
    const std::vector<Spring> &getSprings() const;
    float getCutThreshold() const;
    float getTensionBreaking() const;
    bool getEnableTensionBreaking() const;
    float getTensionBreakThreshold() const;
    AnalysisDisplayData getAnalysisDisplayData() const;
    ClothOrientation getOrientation() const;

  private:
    // Dimension data
    int resX, resY;
    float width, height;
    float floorY = 0.0f;

    // Phycis data
    float simulationTime = 0.0f;
    float tensionBreakThreshold = 3.5f;
    float cutThresholdPixels = 10.0f;
    float correctionFactor = 0.15f;
    float maxStretchRatio = 1.2f;

    // Default springs value
    float defaultMass = 0.5f;
    float defaultStructuralStiffness = 100.0f;
    float defaultStructuralDamping = 2.5f;
    float defaultShearStiffness = 200.0f;
    float defaultShearDamping = 1.5f;
    float defaultBendingStiffness = 100.0f;
    float defaultBendingDamping = 1.2f;

    // Cloth data
    std::vector<Mass> masses;
    std::vector<Spring> springs;
    std::vector<int> massIndexMap;
    std::vector<Object *> collisionObjects;

    // Rendering data
    std::vector<float> massesVertices;
    std::vector<float> lineVertices;
    std::vector<float> textureVertices;
    std::vector<unsigned int> textureIndices;

    // Visual
    bool massVisible = false;
    bool springVisible = false;
    bool textureVisible = true;
    bool enableTensionBreaking = false;
    bool enableCollisions = true;

    // OpenGL data
    unsigned int VAO_masses = 0, VBO_masses = 0;
    unsigned int VAO_lines = 0, VBO_lines = 0;
    unsigned int VAO_texture = 0, VBO_texture = 0, EBO_texture = 0;
    unsigned int textureID = 0;

    int selectedMassIndex = -1;
    // Solver
    int solverIterations = 5;

    // idk
    ClothOrientation currentOrientation;
    ForceManager forceManager;
    ClothAnalysis analysis;
    glm::vec3 lastMouseWorldPos;

    // Setup
    void initCloth();
    void applyConsts();
    void cleanupBuffers();

    // Helpers
    bool springIntersectsSegment(const Spring &spring, const glm::vec3 &segmentStart, const glm::vec3 &segmentEnd,
                                 glm::vec3 &intersectionPoint);
    bool areSpringMidpointsConnected(int springA, int springB) const;
    void rebuildTextureData();
    void rebuildGraphicsData();
};
