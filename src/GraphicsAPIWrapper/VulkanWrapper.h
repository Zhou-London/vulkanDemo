#pragma once

#include "Sphere.h"
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
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;

    uint32_t graphicsFamily;
    uint32_t presentFamily;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkExtent2D extent;

    VkSwapchainKHR swapchain;
    std::vector<VkImageView> swapchainImageViews;
    VkRenderPass renderPass;
    std::vector<VkFramebuffer> swapchainFrameBuffers;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
    VkPipeline graphicsPipeline;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;

    struct BufferData {
      VkBuffer vertexBuffer;
      VkDeviceMemory vertexBufferMemory;

      VkBuffer indexBuffer;
      VkDeviceMemory indexBufferMemory;
    } bufferData;
  };
  struct Params {
    std::string applicationName;
    uint32_t targetGpu;
    GLFWwindow* window;
    uint32_t imageCount;
    VkFormat format;
    VkPresentModeKHR presentMode;
    VkImageViewType imageViewType;

    struct ImageViewSubsource {
      VkImageAspectFlags mask;
      uint32_t mipLevel;
      uint32_t levelCount;
      uint32_t arrayLayer;
      uint32_t layerCount;
    } imageViewSubsource;

    struct ColorAttachment {
      VkSampleCountFlagBits samples;
    } colorAttachment;

    struct RasterizerConfig {
      VkPolygonMode polygonMode;
      VkCullModeFlags cullMode;
      VkFrontFace frontFace;
    } rasterizerConfig;

    Sphere sphere;
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
  bool make_vertex_buffer();
  bool make_index_buffer();

  bool make_pipeline();
  bool record_command_buffers();

  bool make_buffer(VkDeviceSize size,
                   VkBufferUsageFlags usage,
                   VkMemoryPropertyFlags properties,
                   VkBuffer& buffer,
                   VkDeviceMemory& bufferMemory);

  bool init_sync();

  bool init();
  void run();

  const Params& params() const noexcept { return params_; }

  const Data& data() const noexcept { return data_; }

 private:
  Data data_;
  const Params params_;
};