#include "config.h"

#include "VulkanWrapper.h"
#include "shader_util.h"
#include "vulkan_util.h"
#include "UBO.h"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>

VulkanWrapper::VulkanWrapper(Params&& params, Data&& data)
    : params_(std::move(params)), data_(std::move(data)) {}

void VulkanWrapper::make_instance() {
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

  if (vkCreateInstance(&createInfo, nullptr, &data_.instance) != VK_SUCCESS ||
      data_.instance == nullptr) {
    throw std::runtime_error("Failed to create instance!");
  }
}

void VulkanWrapper::make_surface() {
  if (glfwCreateWindowSurface(data_.instance,
                              params_.window,
                              nullptr,
                              &data_.surface) != VK_SUCCESS ||
      data_.surface == nullptr) {
    throw std::runtime_error("Failed to create window surface!");
  }
}

void VulkanWrapper::make_logical_device() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(data_.instance, &deviceCount, nullptr);

  if (deviceCount == 0)
    throw std::runtime_error("Failed to find GPUs with Vulkan support!");

  auto physicalDevices = std::vector<VkPhysicalDevice>(deviceCount);
  vkEnumeratePhysicalDevices(data_.instance,
                             &deviceCount,
                             physicalDevices.data());
  data_.physicalDevice = physicalDevices[params_.targetGpu];

  if (!util::createVkGPU(data_.physicalDevice,
                         data_.surface,
                         &data_.graphicsFamily,
                         &data_.presentFamily,
                         &data_.logicalDevice,
                         &data_.graphicsQueue,
                         &data_.presentQueue) ||
      data_.logicalDevice == nullptr) {
    throw std::runtime_error("Failed to create logical device!");
  }
}

void VulkanWrapper::make_swapchain() {
  auto swapchainSupport =
      util::querySwapchainSupport(data_.physicalDevice, data_.surface);

  auto surfaceFormat =
      util::chooseSwapSurfaceFormat(swapchainSupport.formats, params_.format);

  if (surfaceFormat.format != params_.format) {
    throw std::runtime_error("Failed to make swapchain surface format!");
  }

  auto presentMode = util::chooseSwapPresentMode(swapchainSupport.presentModes,
                                                 params_.presentMode);

  if (presentMode != params_.presentMode) {
    throw std::runtime_error("Failed to make swapchain present mode!");
  }

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

  if (vkCreateSwapchainKHR(data_.logicalDevice,
                           &swapchainCreateInfo,
                           nullptr,
                           &data_.swapchain) != VK_SUCCESS ||
      data_.swapchain == nullptr) {
    throw std::runtime_error("Failed to create swapchain!");
  }
}

void VulkanWrapper::make_swapchain_image_views() {
  auto swapchainImages = std::vector<VkImage>(params_.imageCount);

  uint32_t imageCount = params_.imageCount;
  vkGetSwapchainImagesKHR(data_.logicalDevice,
                          data_.swapchain,
                          &imageCount,
                          swapchainImages.data());

  if (swapchainImages.size() != params_.imageCount) {
    throw std::runtime_error("Failed to get swapchain images!");
  }

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

  if (data_.swapchainImageViews.size() != params_.imageCount) {
    throw std::runtime_error("Failed to create swapchain image views!");
  }
}

void VulkanWrapper::make_depth_resources() {
  auto depthFormat = util::findDepthFormat(data_.physicalDevice);

  auto imageInfo = VkImageCreateInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = depthFormat,
      .extent{
          .width = data_.extent.width,
          .height = data_.extent.height,
          .depth = 1,
      },
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };

  if (vkCreateImage(data_.logicalDevice,
                    &imageInfo,
                    nullptr,
                    &data_.depthData.depthImage) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create depth image!");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(data_.logicalDevice,
                               data_.depthData.depthImage,
                               &memRequirements);

  auto allocInfo =
      VkMemoryAllocateInfo{.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                           .allocationSize = memRequirements.size,

                           .memoryTypeIndex = util::find_memory_type(
                               data_.physicalDevice,
                               memRequirements.memoryTypeBits,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

  if (vkAllocateMemory(data_.logicalDevice,
                       &allocInfo,
                       nullptr,
                       &data_.depthData.depthImageMemory) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate depth image memory!");
  }

  vkBindImageMemory(data_.logicalDevice,
                    data_.depthData.depthImage,
                    data_.depthData.depthImageMemory,
                    0);

  auto imageViewInfo = VkImageViewCreateInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = data_.depthData.depthImage,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = depthFormat,
      .subresourceRange{
          .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
      },
  };

  if (vkCreateImageView(data_.logicalDevice,
                        &imageViewInfo,
                        nullptr,
                        &data_.depthData.depthImageView) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create depth image view!");
  }
}

void VulkanWrapper::make_render_pass() {
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

  auto depthFormat = util::findDepthFormat(data_.physicalDevice);

  auto depthAttachment = VkAttachmentDescription{
      .format = depthFormat,
      .samples = params_.colorAttachment.samples,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };

  auto depthAttachmentRef = VkAttachmentReference{
      .attachment = 1,
      .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  std::array<VkAttachmentDescription, 2> attachments = {colorAttachment,
                                                        depthAttachment};

  auto subpass = VkSubpassDescription{
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachmentRef,
      .pDepthStencilAttachment = &depthAttachmentRef,

  };

  auto dependency = VkSubpassDependency{
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,

      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,

      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                       VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};

  auto renderPassInfo = VkRenderPassCreateInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = static_cast<uint32_t>(attachments.size()),
      .pAttachments = attachments.data(),
      .subpassCount = 1,
      .pSubpasses = &subpass,
      .dependencyCount = 1,
      .pDependencies = &dependency};

  if (vkCreateRenderPass(data_.logicalDevice,
                         &renderPassInfo,
                         nullptr,
                         &data_.renderPass) != VK_SUCCESS ||
      data_.renderPass == nullptr) {
    throw std::runtime_error("Failed to create render pass!");
  }
}

void VulkanWrapper::make_frame_buffers() {
  data_.swapchainFrameBuffers.resize(data_.swapchainImageViews.size());

  for (auto i : std::views::iota(0u, data_.swapchainImageViews.size())) {
    std::array<VkImageView, 2> attachments = {data_.swapchainImageViews[i],
                                              data_.depthData.depthImageView};

    auto frameBufferInfo = VkFramebufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = data_.renderPass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = data_.extent.width,
        .height = data_.extent.height,
        .layers = 1,
    };

    if (vkCreateFramebuffer(data_.logicalDevice,
                            &frameBufferInfo,
                            nullptr,
                            &data_.swapchainFrameBuffers[i]) != VK_SUCCESS)
      throw std::runtime_error("Failed to create frame buffer!");
  }
}

void VulkanWrapper::make_command_pool() {
  auto poolInfo = VkCommandPoolCreateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = data_.graphicsFamily,
  };

  if (vkCreateCommandPool(data_.logicalDevice,
                          &poolInfo,
                          nullptr,
                          &data_.commandPool) != VK_SUCCESS ||
      data_.commandPool == nullptr) {
    throw std::runtime_error("Failed to create command pool!");
  }
}

void VulkanWrapper::make_command_buffers() {
  data_.commandBuffers.resize(data_.swapchainFrameBuffers.size());

  auto allocInfo = VkCommandBufferAllocateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = data_.commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = static_cast<uint32_t>(data_.commandBuffers.size()),
  };

  if (vkAllocateCommandBuffers(data_.logicalDevice,
                               &allocInfo,
                               data_.commandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate command buffers!");
  }
}

void VulkanWrapper::load_shader() {
  try {
    auto vertCode =
        util::readSpvFile(std::string(SHADER_PATH) + "/simple.vert.spv");
    auto fragCode =
        util::readSpvFile(std::string(SHADER_PATH) + "/simple.frag.spv");

    data_.vertShaderModule =
        util::createShaderModule(data_.logicalDevice, vertCode);
    data_.fragShaderModule =
        util::createShaderModule(data_.logicalDevice, fragCode);
  } catch (std::exception& e) {
    throw std::runtime_error(std::string("Failed to load shader: ") + e.what());
  }
}

void VulkanWrapper::make_pipeline() {
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

  auto depthStencil = VkPipelineDepthStencilStateCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
      .depthCompareOp = VK_COMPARE_OP_LESS,
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE};

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
    throw std::runtime_error("Failed to create pipeline layout!");
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
      .pDepthStencilState = &depthStencil,
      .pColorBlendState = &colorBlending,
      .pDynamicState = nullptr,

      .layout = data_.pipelineLayout,
      .renderPass = data_.renderPass,
      .subpass = 0,
  };

  if (vkCreateGraphicsPipelines(data_.logicalDevice,
                                VK_NULL_HANDLE,
                                1,
                                &pipelineInfo,
                                nullptr,
                                &data_.graphicsPipeline) != VK_SUCCESS ||
      data_.graphicsPipeline == nullptr) {
    throw std::runtime_error("Failed to create graphics pipeline!");
  }
}

void VulkanWrapper::record_command_buffers() {
  for (size_t i = 0; i < data_.commandBuffers.size(); ++i) {
    auto beginInfo = VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    vkBeginCommandBuffer(data_.commandBuffers[i], &beginInfo);

    auto clearColor = VkClearValue{
        .color = {{0.1f, 0.1f, 0.15f, 1.0f}},
    };

    VkClearValue clearDepth = {{{1.0f, 0}}};

    std::array<VkClearValue, 2> clearValues = {clearColor, clearDepth};

    auto renderPassInfo = VkRenderPassBeginInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = data_.renderPass,
        .framebuffer = data_.swapchainFrameBuffers[i],
        .renderArea{
            .offset = {0, 0},
            .extent = data_.extent,
        },
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data(),
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
                     static_cast<uint32_t>(params_.model->indices.size()),
                     1,
                     0,
                     0,
                     1);

    vkCmdEndRenderPass(data_.commandBuffers[i]);

    if (vkEndCommandBuffer(data_.commandBuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("Failed to record command buffer!");
    }
  }
}

void VulkanWrapper::make_semaphores() {
  auto semaphoreInfo = VkSemaphoreCreateInfo{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  if (vkCreateSemaphore(data_.logicalDevice,
                        &semaphoreInfo,
                        nullptr,
                        &data_.imageAvailableSemaphore) != VK_SUCCESS ||
      vkCreateSemaphore(data_.logicalDevice,
                        &semaphoreInfo,
                        nullptr,
                        &data_.renderFinishedSemaphore) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create semaphores!");
  }
}

void VulkanWrapper::make_vertex_buffer() {
  VkDeviceSize bufferSize =
      sizeof(params_.model->vertices[0]) * params_.model->vertices.size();

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

  if (result != VK_SUCCESS)
    throw std::runtime_error("Failed to make vertex buffer!");

  memcpy(data, params_.model->vertices.data(), bufferSize);

  vkUnmapMemory(data_.logicalDevice, data_.bufferData.vertexBufferMemory);
}

void VulkanWrapper::make_index_buffer() {
  VkDeviceSize bufferSize =
      sizeof(params_.model->indices[0]) * params_.model->indices.size();

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

  if (result != VK_SUCCESS)
    throw std::runtime_error("Failed to make index buffer!");

  memcpy(data, params_.model->indices.data(), (size_t)bufferSize);

  vkUnmapMemory(data_.logicalDevice, data_.bufferData.indexBufferMemory);
}

void VulkanWrapper::make_uniform_buffers() {
  VkDeviceSize bufferSize = sizeof(UBO);
  size_t imageCount = data_.swapchainImageViews.size();  // 比如 3 张

  data_.bufferData.uniformBuffers.resize(imageCount);
  data_.bufferData.uniformBufferMemory.resize(imageCount);

  for (size_t i = 0; i < imageCount; i++) {
    make_buffer(bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                data_.bufferData.uniformBuffers[i],
                data_.bufferData.uniformBufferMemory[i]);
  }
}

void VulkanWrapper::make_descriptor_pool() {
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

  if (vkCreateDescriptorPool(data_.logicalDevice,
                             &poolInfo,
                             nullptr,
                             &data_.descriptData.descriptorPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor pool!");
  }
}

void VulkanWrapper::make_descriptor_sets() {
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

  if (data_.descriptData.descriptorSets.size() != imageCount) {
    throw std::runtime_error("Failed to create descriptor sets!");
  }
}

void VulkanWrapper::make_descriptor_set_layout() {
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

  if (vkCreateDescriptorSetLayout(data_.logicalDevice,
                                  &uboLayoutInfo,
                                  nullptr,
                                  &data_.descriptData.descriptorSetLayout) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor set layout!");
  }
}

void VulkanWrapper::make_buffer(VkDeviceSize size,
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

  if (vkAllocateMemory(data_.logicalDevice,
                       &allocInfo,
                       nullptr,
                       &bufferMemory) != VK_SUCCESS ||
      vkBindBufferMemory(data_.logicalDevice, buffer, bufferMemory, 0) !=
          VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate buffer memory!");
  }
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

      .proj = glm::perspective(glm::radians(45.0f),
                               static_cast<float>(data_.extent.width) /
                                   static_cast<float>(data_.extent.height),
                               0.1f,
                               10.0f),
  };

  ubo.proj[1][1] *= -1;

  void* data;
  if (vkMapMemory(data_.logicalDevice,
                  data_.bufferData.uniformBufferMemory[currentImage],
                  0,
                  sizeof(ubo),
                  0,
                  &data)) {
    throw std::runtime_error("Failed to update uniform buffer!");
  }

  memcpy(data, &ubo, sizeof(ubo));

  vkUnmapMemory(data_.logicalDevice,
                data_.bufferData.uniformBufferMemory[currentImage]);
}

void VulkanWrapper::init() {
  make_instance();
  make_surface();

  make_logical_device();

  make_swapchain();
  make_swapchain_image_views();
  make_depth_resources();
  make_render_pass();
  make_frame_buffers();

  make_command_pool();
  make_command_buffers();

  load_shader();

  make_vertex_buffer();
  make_index_buffer();
  make_uniform_buffers();

  make_descriptor_set_layout();
  make_descriptor_pool();
  make_descriptor_sets();

  make_pipeline();

  record_command_buffers();

  make_semaphores();
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

void VulkanWrapper::clean_up() { return; }