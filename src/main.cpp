#include "GraphicsAPIWrapper.h"
#include "VulkanWrapper.h"
#include <GLFW/glfw3.h>

#include <cstdint>
#include <glm/glm.hpp>
#include <iostream>

#include "GlfwWindowConfig.h"
#include "config.h"

int main() {
  GlfwWindowConfig glfwWindowConfig;

  if (!glfwWindowConfig.init() ||
      !glfwWindowConfig.createWindow(800, 600, "Vulkan Demo"))
    return -1;

#ifdef GRAPHICS_API_VULKAN
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
#else
  // ! Support More APIs
  auto graphicsAPIWrapper = nullptr;
#endif

  if (!graphicsAPIWrapper.make_instance()) {
    std::cerr << "Failed creating instance\n";
    return -1;
  } else if (TRACE_INFO_LOG) {
    std::cout << "Instance created\n";
  }

  if (!graphicsAPIWrapper.make_surface()) {
    std::cerr << "Failed creating surface\n";
    return -1;
  } else if (TRACE_INFO_LOG) {
    std::cout << "Surface created\n";
  }

  if (!graphicsAPIWrapper.make_logical_device()) {
    std::cerr << "Failed creating logical device\n";
    return -1;
  } else if (TRACE_INFO_LOG) {
    std::cout << "Logical device created\n";
  }

  if (!graphicsAPIWrapper.make_swapchain()) {
    std::cerr << "Failed creating swapchain\n";
    return -1;
  } else if (TRACE_INFO_LOG) {
    std::cout << "Swapchain created\n";
  }

  std::cout << "Vulkan Program Started.\n";

  while (!glfwWindowConfig.shouldClose()) {
    glfwWindowConfig.pollEvents();
  }

  if (TRACE_INFO_LOG)
    std::cout << "Vulkan Program Ended.\n";

  return 0;
}