#pragma once

#include "config.h"
#include <GLFW/glfw3.h>

#include <cstdint>
#include <string>
#include <vector>

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
    VkExtent2D extent;

    VkSwapchainKHR swap_chain;
    std::vector<VkImageView> swap_chain_image_views;
    VkRenderPass render_pass;
    std::vector<VkFramebuffer> swap_chain_frame_buffers;

    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> command_buffers;

    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;
    VkPipeline graphics_pipeline;

    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
  };
  struct Params {
    std::string application_name;
    uint32_t target_gpu;
    GLFWwindow* window;
    uint32_t image_count;
    VkFormat format;
    VkPresentModeKHR present_mode;
  };

  VulkanWrapper(Params&& params, Data&& data);
  ~VulkanWrapper() = default;

  bool make_instance();
  bool make_surface();
  bool make_logical_device();
  bool make_swapchain();
  bool make_swapchain_image_views();
  bool make_render_pass();
  bool make_frame_buffers();
  bool make_command_pool();
  bool make_command_buffers();
  bool load_shader();
  bool make_pipeline();
  bool record_command_buffers();

  bool init_sync();
  void run();

  const Params& params() const noexcept { return params_; }

  const Data& data() const noexcept { return data_; }

 private:
  Data data_;
  const Params params_;
};