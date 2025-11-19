#pragma once

#include "config.h"

#include "Vertex.h"
#include <cstdint>
#include <vector>

struct Sphere {
  std::vector<Vertex> vertices;
  std::vector<uint16_t> indices;

  void generateSphere(float radius, int sectorCount, int stackCount);
};
