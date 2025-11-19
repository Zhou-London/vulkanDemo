#pragma once

#include "config.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;

  static VkVertexInputBindingDescription getBindingDescription();

  static std::array<VkVertexInputAttributeDescription, 2>
  getAttributeDescriptions();
};