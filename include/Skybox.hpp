#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

class Shader;

class Skybox
{
  public:
    Skybox(const std::vector<std::string> &faces);
    ~Skybox();
    
    void draw(Shader &shader, const glm::mat4 &view, const glm::mat4 &projection);
    
    void toggleVisibility() { visible = !visible; }
    bool isVisible() const { return visible; }
    
  private:
    unsigned int VAO, VBO;
    unsigned int cubemapTexture;
    bool visible = true;
    
    void setupSkybox();
    unsigned int loadCubemap(const std::vector<std::string> &faces);
};
