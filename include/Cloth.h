#pragma once

#ifndef CLOTH_H
#define CLOTH_H

#include <glm/glm.hpp>
#include <vector>

struct MassPoint {
  glm::vec3 position;
  glm::vec3 prevPosition;
  glm::vec3 acceleration;
  bool pinned = false;
};
struct Spring {
  int p1, p2;
  float restLength;
};

class Cloth {
public:
  std::vector<MassPoint> points;
  std::vector<Spring> springs;

  Cloth(int width, int height, int resX, int resY);

  void simulate(float dt);
  void draw();

private:
  int numRows, numCols;
};

#endif