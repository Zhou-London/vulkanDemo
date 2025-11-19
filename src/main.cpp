#include "config.h"

#include "Models/Sphere.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <iostream>

#include "GlfwWindowConfig.h"
#include "GraphicsAPIWrapper.h"
#include "VulkanWrapper.h"

int main() {
  GlfwWindowConfig glfwWindowConfig;

  if (!glfwWindowConfig.init() ||
      !glfwWindowConfig.createWindow(3840, 2160, "Vulkan Demo"))
    return -1;

  Sphere sphere;
  sphere.generateSphere(1.0f, 64, 64);

#ifdef GRAPHICS_API_VULKAN

  auto graphicsAPIWrapper = GraphicsAPIWrapper<VulkanWrapper>(
      VulkanWrapper::Params{
          .applicationName = "Vulkan Demo",
          .targetGpu = 0,
          .window = glfwWindowConfig.getWindow(),
          .imageCount = 3,
          .format = VK_FORMAT_B8G8R8A8_SRGB,
          .presentMode = VK_PRESENT_MODE_FIFO_KHR,
          .imageViewType = VK_IMAGE_VIEW_TYPE_3D,
          .imageViewSubsource =
              {
                  .mask = VK_IMAGE_ASPECT_COLOR_BIT,
                  .mipLevel = 0,
                  .levelCount = 1,
                  .arrayLayer = 0,
                  .layerCount = 1,
              },
          .colorAttachment =
              {
                  .samples = VK_SAMPLE_COUNT_1_BIT,
              },
          .rasterizerConfig =
              {
                  .polygonMode = VK_POLYGON_MODE_FILL,
                  .cullMode = VK_CULL_MODE_BACK_BIT,
                  .frontFace = VK_FRONT_FACE_CLOCKWISE,
              },
          .sphere = sphere,
      },
      VulkanWrapper::Data{
          .instance = nullptr,
          .surface = nullptr,

          .physicalDevice = nullptr,
          .logicalDevice = nullptr,

          .graphicsFamily = UINT32_MAX,
          .presentFamily = UINT32_MAX,
          .graphicsQueue = nullptr,
          .presentQueue = nullptr,

          .extent = {.width = 0, .height = 0},

          .swapchain = nullptr,
          .renderPass = nullptr,
          .commandPool = nullptr,

          .vertShaderModule = nullptr,
          .fragShaderModule = nullptr,
          .graphicsPipeline = nullptr,
          .pipelineLayout = nullptr,

          .imageAvailableSemaphore = nullptr,
          .renderFinishedSemaphore = nullptr,

          .bufferData =
              {
                  .vertexBuffer = nullptr,
                  .vertexBufferMemory = nullptr,

                  .indexBuffer = nullptr,
                  .indexBufferMemory = nullptr,

                  .uniformBuffers = {},
                  .uniformBufferMemory = {},
              },

          .descriptData =
              {
                  .descriptorSetLayout = nullptr,
                  .descriptorPool = nullptr,
                  .descriptorSets = {},
              },

          .depthData =
              {
                  .depthImage = nullptr,
                  .depthImageMemory = nullptr,
                  .depthImageView = nullptr,
              },
      });
#else
  // ! Support More APIs
  auto graphicsAPIWrapper = nullptr;
#endif

  try {
    if (graphicsAPIWrapper.init()) {
      std::cout << "Vulkan Program Started.\n";
    } else {
      std::cout << "Init error.\n";
      return -1;
    }

    while (!glfwWindowConfig.shouldClose()) {
      glfwWindowConfig.pollEvents();

      graphicsAPIWrapper.run();
    }

    if (TRACE_INFO_LOG) std::cout << "Vulkan Program Ended.\n";

  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return -1;
  }
  return 0;
}