#include "GraphicsAPIWrapper/GraphicsAPIWrapper.h"
#include "VulkanWrapper.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdint>
#include <glm/glm.hpp>
#include <iostream>

#include "GlfwWindowConfig.h"

int main() {
  GlfwWindowConfig glfwWindowConfig;

  if (!glfwWindowConfig.init() ||
      !glfwWindowConfig.createWindow(800, 600, "Vulkan Demo"))
    return -1;

  auto graphicsAPIWrapper = GraphicsAPIWrapper<VulkanWrapper>(
      VulkanWrapper::Params{
          .application_name = "Vulkan Demo",
          .target_gpu = 0,
          .window = glfwWindowConfig.getWindow(),
      },
      VulkanWrapper::Data{
          .instance = nullptr,
          .surface = nullptr,
          .physical_device = nullptr,
          .logical_device = nullptr,
          .graphics_family = UINT32_MAX,
          .present_family = UINT32_MAX,
          .graphics_queue = nullptr,
          .present_queue = nullptr,
          .swap_chain = nullptr,
      });

  if (!graphicsAPIWrapper.make_instance()) {
    std::cerr << "Failed creating instance\n";
    return -1;
  }

  if (!graphicsAPIWrapper.make_surface()) {
    std::cerr << "Failed creating surface\n";
    return -1;
  }

  if (!graphicsAPIWrapper.make_logical_device()) {
    std::cerr << "Failed creating logical device\n";
    return -1;
  }

  if (!graphicsAPIWrapper.make_swapchain()) {
    std::cerr << "Failed creating swapchain\n";
    return -1;
  }

  std::cout << "Vulkan Program Started.\n";

  while (!glfwWindowConfig.shouldClose()) {
    glfwWindowConfig.pollEvents();
  }

  return 0;
}