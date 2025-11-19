#include "VulkanWrapper.h"
#include "config.h"
#include "shader_util.h"
#include "vulkan_util.h"
#include "UBO.h"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <ranges>
#include <string>
#include <vector>

VulkanWrapper::VulkanWrapper(Params&& params, Data&& data)
    : params_(std::move(params)), data_(std::move(data)) {}

bool VulkanWrapper::make_instance() {
  auto appInfo = VkApplicationInfo{
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "VulkanDemo",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "No Engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_0,
  };

  uint32_t extensionCount = 0;
  auto extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

  auto createInfo = VkInstanceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &appInfo,

      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,

      .enabledExtensionCount = extensionCount,
      .ppEnabledExtensionNames = extensions,
  };
  return vkCreateInstance(&createInfo, nullptr, &data_.instance) ==
             VK_SUCCESS &&
         data_.instance != nullptr;
}

bool VulkanWrapper::make_surface() {
  return glfwCreateWindowSurface(data_.instance,
                                 params_.window,
                                 nullptr,
                                 &data_.surface) == VK_SUCCESS &&
         data_.surface != nullptr;
}

bool VulkanWrapper::make_logical_device() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(data_.instance, &deviceCount, nullptr);
  if (deviceCount == 0) {
    return false;
  }

  auto physicalDevices = std::vector<VkPhysicalDevice>(deviceCount);
  vkEnumeratePhysicalDevices(data_.instance,
                             &deviceCount,
                             physicalDevices.data());
  data_.physicalDevice = physicalDevices[params_.targetGpu];

  return util::createVkGPU(data_.physicalDevice,
                           data_.surface,
                           &data_.graphicsFamily,
                           &data_.presentFamily,
                           &data_.logicalDevice,
                           &data_.graphicsQueue,
                           &data_.presentQueue) &&
         data_.logicalDevice != nullptr;
}

bool VulkanWrapper::make_swapchain() {
  auto swapchainSupport =
      util::querySwapchainSupport(data_.physicalDevice, data_.surface);

  auto surfaceFormat =
      util::chooseSwapSurfaceFormat(swapchainSupport.formats, params_.format);

  if (surfaceFormat.format != params_.format) return false;

  auto presentMode = util::chooseSwapPresentMode(swapchainSupport.presentModes,
                                                 params_.presentMode);

  if (presentMode != params_.presentMode) return false;

  data_.extent = util::chooseSwapExtent(swapchainSupport.cap, params_.window);

  auto swapchainCreateInfo =
      util::generate_swapchain_create_info(data_.surface,
                                           surfaceFormat,
                                           params_.imageCount,
                                           data_.extent,
                                           presentMode,
                                           data_.graphicsFamily,
                                           data_.presentFamily,
                                           swapchainSupport.cap);

  return vkCreateSwapchainKHR(data_.logicalDevice,
                              &swapchainCreateInfo,
                              nullptr,
                              &data_.swapchain) == VK_SUCCESS &&
         data_.swapchain != nullptr;
}

bool VulkanWrapper::make_swapchain_image_views() {
  auto swapchainImages = std::vector<VkImage>(params_.imageCount);

  uint32_t imageCount = params_.imageCount;
  vkGetSwapchainImagesKHR(data_.logicalDevice,
                          data_.swapchain,
                          &imageCount,
                          swapchainImages.data());

  if (swapchainImages.size() != params_.imageCount) return false;

  for (auto image : swapchainImages) {
    auto viewInfo = VkImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .image = image,
        .viewType = params_.imageViewType,
        .format = params_.format,
        .components{
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
        },
        .subresourceRange{
            .aspectMask = params_.imageViewSubsource.mask,
            .baseMipLevel = params_.imageViewSubsource.mipLevel,
            .levelCount = params_.imageViewSubsource.levelCount,
            .baseArrayLayer = params_.imageViewSubsource.arrayLayer,
            .layerCount = params_.imageViewSubsource.layerCount,
        }};

    VkImageView imageView;
    vkCreateImageView(data_.logicalDevice, &viewInfo, nullptr, &imageView);

    data_.swapchainImageViews.push_back(imageView);
  }

  if (data_.swapchainImageViews.size() != params_.imageCount) return false;

  return true;
}

bool VulkanWrapper::make_render_pass() {
  auto colorAttachment = VkAttachmentDescription{
      .format = params_.format,
      .samples = params_.colorAttachment.samples,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};

  auto colorAttachmentRef =
      VkAttachmentReference{.attachment = 0,
                            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  auto subpass =
      VkSubpassDescription{.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                           .colorAttachmentCount = 1,
                           .pColorAttachments = &colorAttachmentRef};

  auto dependency = VkSubpassDependency{
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,

      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,

      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};

  auto renderPassInfo =
      VkRenderPassCreateInfo{.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                             .attachmentCount = 1,
                             .pAttachments = &colorAttachment,
                             .subpassCount = 1,
                             .pSubpasses = &subpass,
                             .dependencyCount = 1,
                             .pDependencies = &dependency};

  return vkCreateRenderPass(data_.logicalDevice,
                            &renderPassInfo,
                            nullptr,
                            &data_.renderPass) == VK_SUCCESS &&
         data_.renderPass != nullptr;
}

bool VulkanWrapper::make_frame_buffers() {
  data_.swapchainFrameBuffers.resize(data_.swapchainImageViews.size());

  for (auto i : std::views::iota(0u, data_.swapchainImageViews.size())) {
    VkImageView attachments[] = {data_.swapchainImageViews[i]};

    auto frameBufferInfo = VkFramebufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = data_.renderPass,
        .attachmentCount = 1,
        .pAttachments = attachments,
        .width = data_.extent.width,
        .height = data_.extent.height,
        .layers = 1,
    };

    if (vkCreateFramebuffer(data_.logicalDevice,
                            &frameBufferInfo,
                            nullptr,
                            &data_.swapchainFrameBuffers[i]) != VK_SUCCESS)
      return false;
  }

  return true;
}

bool VulkanWrapper::make_command_pool() {
  auto poolInfo = VkCommandPoolCreateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = data_.graphicsFamily,
  };

  return vkCreateCommandPool(data_.logicalDevice,
                             &poolInfo,
                             nullptr,
                             &data_.commandPool) == VK_SUCCESS &&
         data_.commandPool != nullptr;
}

bool VulkanWrapper::make_command_buffers() {
  data_.commandBuffers.resize(data_.swapchainFrameBuffers.size());

  auto allocInfo = VkCommandBufferAllocateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = data_.commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = static_cast<uint32_t>(data_.commandBuffers.size()),
  };

  return vkAllocateCommandBuffers(data_.logicalDevice,
                                  &allocInfo,
                                  data_.commandBuffers.data()) == VK_SUCCESS;
}

bool VulkanWrapper::load_shader() {
  auto vertCode =
      util::readSpvFile(std::string(SHADER_PATH) + "/simple.vert.spv");
  auto fragCode =
      util::readSpvFile(std::string(SHADER_PATH) + "/simple.frag.spv");

  data_.vertShaderModule =
      util::createShaderModule(data_.logicalDevice, vertCode);
  data_.fragShaderModule =
      util::createShaderModule(data_.logicalDevice, fragCode);

  return true;
}

bool VulkanWrapper::make_pipeline() {
  auto vertShaderStageInfo = VkPipelineShaderStageCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = data_.vertShaderModule,
      .pName = "main",
  };

  auto bindingDescription = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  auto vertexInputInfo = VkPipelineVertexInputStateCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,

      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &bindingDescription,

      .vertexAttributeDescriptionCount =
          static_cast<uint32_t>(attributeDescriptions.size()),
      .pVertexAttributeDescriptions = attributeDescriptions.data(),
  };
  auto fragShaderStageInfo = VkPipelineShaderStageCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = data_.fragShaderModule,
      .pName = "main",
  };

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                    fragShaderStageInfo};

  auto inputAssembly = VkPipelineInputAssemblyStateCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
  };

  auto viewPort = VkViewport{
      .x = 0.0f,
      .y = 0.0f,
      .width = static_cast<float>(data_.extent.width),
      .height = static_cast<float>(data_.extent.height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };
  auto scissor = VkRect2D{
      .offset = {0, 0},
      .extent = data_.extent,
  };
  auto viewPortState = VkPipelineViewportStateCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .pViewports = &viewPort,
      .scissorCount = 1,
      .pScissors = &scissor,
  };

  auto rasterizer = VkPipelineRasterizationStateCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = params_.rasterizerConfig.polygonMode,
      .cullMode = params_.rasterizerConfig.cullMode,
      .frontFace = params_.rasterizerConfig.frontFace,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1.0f,
  };

  auto multisampling = VkPipelineMultisampleStateCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = params_.colorAttachment.samples,
      .sampleShadingEnable = VK_FALSE,
  };

  auto colorBlendAttachment = VkPipelineColorBlendAttachmentState{
      .blendEnable = VK_FALSE,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };

  auto colorBlending = VkPipelineColorBlendStateCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .attachmentCount = 1,
      .pAttachments = &colorBlendAttachment,
      .blendConstants{0.0f, 0.0f, 0.0f, 0.0f},
  };

  auto pipelineLayoutInfo = VkPipelineLayoutCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &data_.descriptData.descriptorSetLayout,
      .pushConstantRangeCount = 0,
  };

  if (vkCreatePipelineLayout(data_.logicalDevice,
                             &pipelineLayoutInfo,
                             nullptr,
                             &data_.pipelineLayout) != VK_SUCCESS) {
    return false;
  }

  auto pipelineInfo = VkGraphicsPipelineCreateInfo{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = 2,
      .pStages = shaderStages,

      .pVertexInputState = &vertexInputInfo,
      .pInputAssemblyState = &inputAssembly,
      .pViewportState = &viewPortState,
      .pRasterizationState = &rasterizer,
      .pMultisampleState = &multisampling,
      .pDepthStencilState = nullptr,
      .pColorBlendState = &colorBlending,
      .pDynamicState = nullptr,

      .layout = data_.pipelineLayout,
      .renderPass = data_.renderPass,
      .subpass = 0,
  };

  return vkCreateGraphicsPipelines(data_.logicalDevice,
                                   VK_NULL_HANDLE,
                                   1,
                                   &pipelineInfo,
                                   nullptr,
                                   &data_.graphicsPipeline) == VK_SUCCESS &&
         data_.graphicsPipeline != nullptr;
}

bool VulkanWrapper::record_command_buffers() {
  for (size_t i = 0; i < data_.commandBuffers.size(); ++i) {
    auto beginInfo = VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    vkBeginCommandBuffer(data_.commandBuffers[i], &beginInfo);

    auto clearColor = VkClearValue{
        .color = {{0.1f, 0.1f, 0.15f, 1.0f}},
    };

    auto renderPassInfo = VkRenderPassBeginInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = data_.renderPass,
        .framebuffer = data_.swapchainFrameBuffers[i],
        .renderArea{
            .offset = {0, 0},
            .extent = data_.extent,
        },
        .clearValueCount = 1,
        .pClearValues = &clearColor,
    };

    vkCmdBeginRenderPass(data_.commandBuffers[i],
                         &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(data_.commandBuffers[i],
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      data_.graphicsPipeline);

    VkBuffer vertexBuffers[] = {data_.bufferData.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(data_.commandBuffers[i],
                           0,
                           1,
                           vertexBuffers,
                           offsets);

    vkCmdBindIndexBuffer(data_.commandBuffers[i],
                         data_.bufferData.indexBuffer,
                         0,
                         VK_INDEX_TYPE_UINT16);

    vkCmdBindDescriptorSets(data_.commandBuffers[i],
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            data_.pipelineLayout,
                            0,
                            1,
                            &data_.descriptData.descriptorSets[i],
                            0,
                            nullptr);

    vkCmdDrawIndexed(data_.commandBuffers[i],
                     static_cast<uint32_t>(params_.sphere.indices.size()),
                     1,
                     0,
                     0,
                     1);

    vkCmdEndRenderPass(data_.commandBuffers[i]);

    if (vkEndCommandBuffer(data_.commandBuffers[i]) != VK_SUCCESS) {
      return false;
    }
  }

  return true;
}

bool VulkanWrapper::init_sync() {
  auto semaphoreInfo = VkSemaphoreCreateInfo{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  return vkCreateSemaphore(data_.logicalDevice,
                           &semaphoreInfo,
                           nullptr,
                           &data_.imageAvailableSemaphore) == VK_SUCCESS &&
         vkCreateSemaphore(data_.logicalDevice,
                           &semaphoreInfo,
                           nullptr,
                           &data_.renderFinishedSemaphore) == VK_SUCCESS;
}

bool VulkanWrapper::make_vertex_buffer() {
  VkDeviceSize bufferSize =
      sizeof(params_.sphere.vertices[0]) * params_.sphere.vertices.size();

  make_buffer(bufferSize,
              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
              data_.bufferData.vertexBuffer,
              data_.bufferData.vertexBufferMemory);

  void* data;
  auto result = vkMapMemory(data_.logicalDevice,
                            data_.bufferData.vertexBufferMemory,
                            0,
                            bufferSize,
                            0,
                            &data);

  memcpy(data, params_.sphere.vertices.data(), bufferSize);

  vkUnmapMemory(data_.logicalDevice, data_.bufferData.vertexBufferMemory);

  return result == VK_SUCCESS;
}

bool VulkanWrapper::make_index_buffer() {
  VkDeviceSize bufferSize =
      sizeof(params_.sphere.indices[0]) * params_.sphere.indices.size();

  make_buffer(bufferSize,
              VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
              data_.bufferData.indexBuffer,
              data_.bufferData.indexBufferMemory);

  void* data;
  auto result = vkMapMemory(data_.logicalDevice,
                            data_.bufferData.indexBufferMemory,
                            0,
                            bufferSize,
                            0,
                            &data);

  memcpy(data, params_.sphere.indices.data(), (size_t)bufferSize);

  vkUnmapMemory(data_.logicalDevice, data_.bufferData.indexBufferMemory);

  return result == VK_SUCCESS;
}

bool VulkanWrapper::make_uniform_buffers() {
  VkDeviceSize bufferSize = sizeof(UBO);
  size_t imageCount = data_.swapchainImageViews.size();  // 比如 3 张

  data_.bufferData.uniformBuffers.resize(imageCount);
  data_.bufferData.uniformBufferMemory.resize(imageCount);

  bool result = true;
  for (size_t i = 0; i < imageCount; i++) {
    result = make_buffer(bufferSize,
                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         data_.bufferData.uniformBuffers[i],
                         data_.bufferData.uniformBufferMemory[i]) &&
             result;
  }

  return result;
}

bool VulkanWrapper::make_descriptor_pool() {
  size_t imageCount = data_.swapchainImageViews.size();
  auto poolSize = VkDescriptorPoolSize{
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = static_cast<uint32_t>(imageCount),
  };

  auto poolInfo = VkDescriptorPoolCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = static_cast<uint32_t>(imageCount),
      .poolSizeCount = 1,
      .pPoolSizes = &poolSize,
  };

  return vkCreateDescriptorPool(data_.logicalDevice,
                                &poolInfo,
                                nullptr,
                                &data_.descriptData.descriptorPool) ==
         VK_SUCCESS;
}

bool VulkanWrapper::make_descriptor_sets() {
  size_t imageCount = data_.swapchainImageViews.size();
  auto layouts = std::vector<VkDescriptorSetLayout>(
      imageCount,
      data_.descriptData.descriptorSetLayout);

  auto allocInfo = VkDescriptorSetAllocateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = data_.descriptData.descriptorPool,
      .descriptorSetCount = static_cast<uint32_t>(imageCount),
      .pSetLayouts = layouts.data(),
  };

  data_.descriptData.descriptorSets.resize(imageCount);
  vkAllocateDescriptorSets(data_.logicalDevice,
                           &allocInfo,
                           data_.descriptData.descriptorSets.data());

  for (auto i : std::ranges::views::iota(0u, imageCount)) {
    auto bufferInfo = VkDescriptorBufferInfo{
        .buffer = data_.bufferData.uniformBuffers[i],
        .offset = 0,
        .range = sizeof(UBO),
    };

    auto descriptorWrite = VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = data_.descriptData.descriptorSets[i],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &bufferInfo,
    };

    vkUpdateDescriptorSets(data_.logicalDevice,
                           1,
                           &descriptorWrite,
                           0,
                           nullptr);
  }

  return data_.descriptData.descriptorSets.size() == imageCount;
}

bool VulkanWrapper::make_descriptor_set_layout() {
  auto uboLayoutBinding = VkDescriptorSetLayoutBinding{
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
  };

  auto uboLayoutInfo = VkDescriptorSetLayoutCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 1,
      .pBindings = &uboLayoutBinding,
  };

  return vkCreateDescriptorSetLayout(data_.logicalDevice,
                                     &uboLayoutInfo,
                                     nullptr,
                                     &data_.descriptData.descriptorSetLayout) ==
         VK_SUCCESS;
}

bool VulkanWrapper::make_buffer(VkDeviceSize size,
                                VkBufferUsageFlags usage,
                                VkMemoryPropertyFlags properties,
                                VkBuffer& buffer,
                                VkDeviceMemory& bufferMemory) {
  auto bufferInfo = VkBufferCreateInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .usage = usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  vkCreateBuffer(data_.logicalDevice, &bufferInfo, nullptr, &buffer);

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(data_.logicalDevice, buffer, &memRequirements);

  auto allocInfo = VkMemoryAllocateInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex = util::find_memory_type(data_.physicalDevice,
                                                memRequirements.memoryTypeBits,
                                                properties),
  };

  return vkAllocateMemory(data_.logicalDevice,
                          &allocInfo,
                          nullptr,
                          &bufferMemory) == VK_SUCCESS

         &&

         vkBindBufferMemory(data_.logicalDevice, buffer, bufferMemory, 0) ==
             VK_SUCCESS;
}

void VulkanWrapper::update_uniform_buffer(uint32_t currentImage) {
  static auto startTime = std::chrono::high_resolution_clock::now();

  auto currentTime = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float, std::chrono::seconds::period>(
                   currentTime - startTime)
                   .count();

  auto ubo = UBO{
      .model = glm::rotate(glm::mat4(1.0f),
                           time * glm::radians(90.0f),
                           glm::vec3(0.0f, 0.0f, 1.0f)),

      .view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
                          glm::vec3(0.0f, 0.0f, 0.0f),
                          glm::vec3(0.0f, 0.0f, 1.0f)),

      .proj = glm::perspective(
          glm::radians(45.0f),
          static_cast<float>(data_.extent.width) / data_.extent.height,
          0.1f,
          10.0f),
  };

  ubo.proj[1][1] *= -1;

  void* data;
  vkMapMemory(data_.logicalDevice,
              data_.bufferData.uniformBufferMemory[currentImage],
              0,
              sizeof(ubo),
              0,
              &data);

  memcpy(data, &ubo, sizeof(ubo));

  vkUnmapMemory(data_.logicalDevice,
                data_.bufferData.uniformBufferMemory[currentImage]);
}

bool VulkanWrapper::init() {
  return make_instance() && make_surface() && make_logical_device() &&
         make_swapchain() && make_swapchain_image_views() &&
         make_render_pass() && make_frame_buffers() && make_command_pool() &&
         make_command_buffers() && load_shader() && make_vertex_buffer() &&
         make_index_buffer() && make_uniform_buffers() &&
         make_descriptor_set_layout() && make_descriptor_pool() &&
         make_descriptor_sets() && make_pipeline() &&
         record_command_buffers() && init_sync();
}

void VulkanWrapper::run() {
  uint32_t imageIndex;

  vkAcquireNextImageKHR(data_.logicalDevice,
                        data_.swapchain,
                        UINT64_MAX,
                        data_.imageAvailableSemaphore,
                        VK_NULL_HANDLE,
                        &imageIndex);

  VkSemaphore waitSemaphores[] = {data_.imageAvailableSemaphore};

  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSemaphore signalSemaphores[] = {data_.renderFinishedSemaphore};

  auto submitInfo = VkSubmitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = waitSemaphores,
      .pWaitDstStageMask = waitStages,

      .commandBufferCount = 1,
      .pCommandBuffers = &data_.commandBuffers[imageIndex],

      .signalSemaphoreCount = 1,
      .pSignalSemaphores = signalSemaphores,
  };

  update_uniform_buffer(imageIndex);

  vkQueueSubmit(data_.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

  VkSwapchainKHR swapchains[] = {data_.swapchain};

  auto presentInfo = VkPresentInfoKHR{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = signalSemaphores,

      .swapchainCount = 1,
      .pSwapchains = swapchains,
      .pImageIndices = &imageIndex,
  };

  vkQueuePresentKHR(data_.presentQueue, &presentInfo);
}