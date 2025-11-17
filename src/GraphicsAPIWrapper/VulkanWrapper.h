#pragma once

#include <memory>
#include <string>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanWrapper {
public:
  struct Data {
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    VkDevice logical_device;
    uint32_t graphics_family;
    uint32_t present_family;
    VkQueue graphics_queue;
    VkQueue present_queue;
    VkSwapchainKHR swap_chain;
  };
  struct Params {
    std::string application_name;
    uint32_t target_gpu;
    GLFWwindow *window;
  };

  VulkanWrapper(Params &&params, Data &&data);
  ~VulkanWrapper() = default;

  bool make_instance();
  bool make_surface();
  bool make_logical_device();
  bool make_swapchain();

  Data *data() const noexcept { return data_.get(); }

private:
  std::unique_ptr<Data> data_;
  std::unique_ptr<Params> params_;
};