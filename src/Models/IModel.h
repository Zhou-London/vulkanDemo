#pragma once

#include <vector>
#include "Vertex.h"

struct IModel {
  std::vector<Vertex> vertices;
  std::vector<uint16_t> indices;

  IModel() { generate(); }

  virtual void generate() {
    vertices.clear();
    indices.clear();
  };
};