
#include "Vertex.h"
#include <array>
#include <cstddef>

VkVertexInputBindingDescription Vertex::getBindingDescription() {
  auto bindingDescription =
      VkVertexInputBindingDescription{.binding = 0,
                                      .stride = sizeof(Vertex),
                                      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};

  return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 2>
Vertex::getAttributeDescriptions() {
  auto attributeDescriptions = std::array<VkVertexInputAttributeDescription, 2>{
      VkVertexInputAttributeDescription{.location = 0,
                                        .binding = 0,
                                        .format = VK_FORMAT_R32G32_SFLOAT,
                                        .offset = offsetof(Vertex, pos)},

      VkVertexInputAttributeDescription{.location = 1,
                                        .binding = 0,
                                        .format = VK_FORMAT_R32G32_SFLOAT,
                                        .offset = offsetof(Vertex, color)}};

  return attributeDescriptions;
}