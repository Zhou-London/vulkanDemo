
#include "Sphere.h"
#include <cmath>
#include <glm/fwd.hpp>
#include <ranges>

void Sphere::generateSphere(float radius, int sectorCount, int stackCount) {
  vertices.clear();
  indices.clear();

  float x, y, z, xy;
  float nx, ny, nz, lengthInv = 1.0f / radius;
  float s, t;

  float sectorStep = 2 * M_PI / sectorCount;
  float stackStep = M_PI / stackCount;
  float sectorAngle, stackAngle;

  for (auto i : std::ranges::views::iota(0, stackCount + 1)) {
    stackAngle = M_PI / 2 - i * stackStep;
    xy = radius * std::cosf(stackAngle);
    z = radius * std::sinf(stackAngle);

    for (auto j : std::ranges::views::iota(0, sectorCount + 1)) {
      sectorAngle = j * sectorStep;

      x = xy * std::cosf(sectorAngle);
      y = xy * std::sinf(sectorAngle);

      float r = (x / radius + 1) / 2;
      float g = (y / radius + 1) / 2;
      float b = (z / radius + 1) / 2;

      vertices.push_back(
          {.pos = glm::vec3(x, y, z), .color = glm::vec3(r, g, b)});
    }
  }

  for (auto i : std::ranges::views::iota(0, stackCount)) {
    int k1 = i * (sectorCount + 1);
    int k2 = k1 + sectorCount + 1;

    for (auto j : std::ranges::views::iota(0, sectorCount)) {
      if (i != 0) {
        indices.push_back(k1);
        indices.push_back(k2);
        indices.push_back(k1 + 1);
      }

      if (i != (stackCount - 1)) {
        indices.push_back(k1 + 1);
        indices.push_back(k2);
        indices.push_back(k2 + 1);
      }

      ++k1;
      ++k2;
    }
  }
};