#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

class Shader;

class Skybox
{
  public:
    Skybox(const std::vector<std::string> &faces);
    ~Skybox();

    // Rendering
    void draw(Shader &shader, const glm::mat4 &view, const glm::mat4 &projection);

    // Visibiliy
    void toggleVisibility()
    {
        visible = !visible;
    }
    bool isVisible() const
    {
        return visible;
    }

  private:
    // Rendering data
    unsigned int VAO, VBO;
    unsigned int cubemapTexture;
    // State
    bool visible = true;

    // Init
    void setupSkybox();
    unsigned int loadCubemap(const std::vector<std::string> &faces);
};
