#pragma once

#include "config.h"
#include "Sphere.h"
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
    VkPipelineLayout pipelineLayout;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;

    struct BufferData {
      VkBuffer vertexBuffer;
      VkDeviceMemory vertexBufferMemory;

      VkBuffer indexBuffer;
      VkDeviceMemory indexBufferMemory;

      std::vector<VkBuffer> uniformBuffers;
      std::vector<VkDeviceMemory> uniformBufferMemory;
    } bufferData;

    struct DescriptData {
      VkDescriptorSetLayout descriptorSetLayout;
      VkDescriptorPool descriptorPool;
      std::vector<VkDescriptorSet> descriptorSets;
    } descriptData;

    struct DepthData {
      VkImage depthImage;
      VkDeviceMemory depthImageMemory;
      VkImageView depthImageView;
    } depthData;
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

    IModel* model;
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
  bool make_uniform_buffers();
  bool make_descriptor_pool();
  bool make_descriptor_sets();
  bool make_descriptor_set_layout();

  bool make_depth_resources();

  bool make_pipeline();
  bool record_command_buffers();

  bool make_buffer(VkDeviceSize size,
                   VkBufferUsageFlags usage,
                   VkMemoryPropertyFlags properties,
                   VkBuffer& buffer,
                   VkDeviceMemory& bufferMemory);

  bool init_sync();

  void update_uniform_buffer(uint32_t currentImage);

  bool init();
  void run();
  void clean_up();

  const Params& params() const noexcept { return params_; }

  const Data& data() const noexcept { return data_; }

 private:
  Data data_;
  const Params params_;
};