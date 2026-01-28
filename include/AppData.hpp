#pragma once

#include <glm/glm.hpp>

// Forward declarations
class Cloth;
class Skybox;
class ClothGUI;
class Camera;
class Cube;

struct AppData
{
    // Core simulation components
    Cloth *cloth;
    Skybox *skybox;
    ClothGUI *gui;
    Camera *camera;

    // Scene objects
    glm::vec3 *lightPos;
    Cube *cube;
    bool *cubeEnabled;

    // Performance metrics
    double fps;
};