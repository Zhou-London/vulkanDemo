#include "config.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <iostream>

#include "GlfwWindowConfig.h"
#include "GraphicsAPIWrapper.h"
#include "VulkanWrapper.h"

int main() {
  GlfwWindowConfig glfwWindowConfig;

  if (!glfwWindowConfig.init() ||
      !glfwWindowConfig.createWindow(1600, 1200, "Vulkan Demo"))
    return -1;

#ifdef GRAPHICS_API_VULKAN
  auto graphicsAPIWrapper = GraphicsAPIWrapper<VulkanWrapper>(
      VulkanWrapper::Params{
          .applicationName = "Vulkan Demo",
          .targetGpu = 0,
          .window = glfwWindowConfig.getWindow(),
          .imageCount = 3,
          .format = VK_FORMAT_B8G8R8A8_SRGB,
          .presentMode = VK_PRESENT_MODE_FIFO_KHR,
          .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
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
      },
      VulkanWrapper::Data{.instance = nullptr,
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
                          .imageAvailableSemaphore = nullptr,
                          .renderFinishedSemaphore = nullptr});
#else
  // ! Support More APIs
  auto graphicsAPIWrapper = nullptr;
#endif

  try {
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

    if (!graphicsAPIWrapper.make_swapchain_image_views()) {
      std::cerr << "Failed creating swapchain image views\n";
      return -1;
    } else if (TRACE_INFO_LOG) {
      std::cout << "Swapchain image views created\n";
    }

    if (!graphicsAPIWrapper.make_render_pass()) {
      std::cerr << "Failed creating render pass\n";
      return -1;
    } else if (TRACE_INFO_LOG) {
      std::cout << "Render pass created\n";
    }

    if (!graphicsAPIWrapper.make_frame_buffers()) {
      std::cerr << "Failed creating frame buffers\n";
      return -1;
    } else if (TRACE_INFO_LOG) {
      std::cout << "Frame buffers created\n";
    }

    if (!graphicsAPIWrapper.make_command_pool()) {
      std::cerr << "Failed creating command pool\n";
      return -1;
    } else if (TRACE_INFO_LOG) {
      std::cout << "Command pool created\n";
    }

    if (!graphicsAPIWrapper.make_command_buffers()) {
      std::cerr << "Failed creating command buffers\n";
      return -1;
    } else if (TRACE_INFO_LOG) {
      std::cout << "Command buffers created\n";
    }

    if (!graphicsAPIWrapper.load_shader()) {
      std::cerr << "Failed loading shader\n";
      return -1;
    } else if (TRACE_INFO_LOG) {
      std::cout << "Shader loaded\n";
    }

    if (!graphicsAPIWrapper.make_pipeline()) {
      std::cerr << "Failed creating pipeline\n";
      return -1;
    } else if (TRACE_INFO_LOG) {
      std::cout << "Pipeline created\n";
    }

    if (!graphicsAPIWrapper.record_command_buffers()) {
      std::cerr << "Failed recording command buffers\n";
      return -1;
    } else if (TRACE_INFO_LOG) {
      std::cout << "Command buffers recorded\n";
    }

    if (!graphicsAPIWrapper.init_sync()) {
      std::cerr << "Failed initializing synchronization objects\n";
      return -1;
    } else if (TRACE_INFO_LOG) {
      std::cout << "Synchronization objects initialized\n";
    }

    std::cout << "Vulkan Program Started.\n";

    while (!glfwWindowConfig.shouldClose()) {
      glfwWindowConfig.pollEvents();

      graphicsAPIWrapper.run();
    }

    if (TRACE_INFO_LOG) std::cout << "Vulkan Program Ended.\n";

  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
  return 0;
}