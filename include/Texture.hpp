#pragma once
#include <glad/glad.h>
#include <string>

class Texture
{
  public:
    unsigned int ID;

    Texture(const char *path);
    void bind(unsigned int slot = 0) const;
    void unbind() const;
};