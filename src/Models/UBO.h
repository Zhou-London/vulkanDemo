#pragma once

#include "config.h"
#include <glm/fwd.hpp>
#include <glm/glm.hpp>

struct UBO {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};