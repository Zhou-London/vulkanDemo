
#include "Sphere.h"
#include <cmath>
#include <glm/fwd.hpp>
#include <ranges>
#include "IModel.h"

Sphere::Sphere() : Sphere(1.0f, 32, 32) {}

Sphere::Sphere(float radius, int sectorCount, int stackCount)
    : radius_(radius),
      sectorCount_(sectorCount),
      stackCount_(stackCount),
      IModel() {
  init();
}

void Sphere::generate() {
  vertices.clear();
  indices.clear();

  float x, y, z, xy;
  float nx, ny, nz, lengthInv = 1.0f / radius_;
  float s, t;

  float sectorStep = 2 * M_PI / sectorCount_;
  float stackStep = M_PI / stackCount_;
  float sectorAngle, stackAngle;

  for (auto i : std::ranges::views::iota(0, stackCount_ + 1)) {
    stackAngle = M_PI / 2 - i * stackStep;
    xy = radius_ * std::cosf(stackAngle);
    z = radius_ * std::sinf(stackAngle);

    for (auto j : std::ranges::views::iota(0, sectorCount_ + 1)) {
      sectorAngle = j * sectorStep;

      x = xy * std::cosf(sectorAngle);
      y = xy * std::sinf(sectorAngle);

      float r = (x / radius_ + 1) / 2;
      float g = (y / radius_ + 1) / 2;
      float b = (z / radius_ + 1) / 2;

      vertices.push_back(
          {.pos = glm::vec3(x, y, z), .color = glm::vec3(r, g, b)});
    }
  }

  for (auto i : std::ranges::views::iota(0, stackCount_)) {
    int k1 = i * (sectorCount_ + 1);
    int k2 = k1 + sectorCount_ + 1;

    for (auto j : std::ranges::views::iota(0, sectorCount_)) {
      if (i != 0) {
        indices.push_back(k1);
        indices.push_back(k2);
        indices.push_back(k1 + 1);
      }

      if (i != (stackCount_ - 1)) {
        indices.push_back(k1 + 1);
        indices.push_back(k2);
        indices.push_back(k2 + 1);
      }

      ++k1;
      ++k2;
    }
  }
};