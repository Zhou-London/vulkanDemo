#pragma once

#include <cstdint>
#include <string>
#include <vector>
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
    std::vector<VkImageView> swap_chain_image_views;
    VkRenderPass render_pass;
  };
  struct Params {
    std::string application_name;
    uint32_t target_gpu;
    GLFWwindow *window;
    uint32_t image_count;
    VkFormat format;
    VkPresentModeKHR present_mode;
  };

  VulkanWrapper(Params &&params, Data &&data);
  ~VulkanWrapper() = default;

  bool make_instance();
  bool make_surface();
  bool make_logical_device();
  bool make_swapchain();
  bool make_swapchain_image_views();
  bool make_render_pass();

  const Params &params() const noexcept { return params_; }

  const Data &data() const noexcept { return data_; }

private:
  Data data_;
  const Params params_;
};