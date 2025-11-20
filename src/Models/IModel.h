#pragma once

#include <exception>
#include <stdexcept>
#include <string>
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

  void init() {
    try {
      generate();
    } catch (std::exception& e) {
      throw std::runtime_error(std::string("Model Error: ") + e.what());
    }
  }
};