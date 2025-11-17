#include "VulkanWrapper.h"
#include "config.h"
#include "shader_util.h"
#include "vulkan_util.h"
#include <cstddef>
#include <cstdint>
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
  data_.physical_device = physicalDevices[params_.target_gpu];

  return util::createVkGPU(data_.physical_device,
                           data_.surface,
                           &data_.graphics_family,
                           &data_.present_family,
                           &data_.logical_device,
                           &data_.graphics_queue,
                           &data_.present_queue) &&
         data_.logical_device != nullptr;
}

bool VulkanWrapper::make_swapchain() {
  auto swapchainSupport =
      util::querySwapchainSupport(data_.physical_device, data_.surface);

  auto surfaceFormat =
      util::chooseSwapSurfaceFormat(swapchainSupport.formats, params_.format);

  if (surfaceFormat.format != params_.format) return false;

  auto presentMode = util::chooseSwapPresentMode(swapchainSupport.presentModes,
                                                 params_.present_mode);

  if (presentMode != params_.present_mode) return false;

  data_.extent = util::chooseSwapExtent(swapchainSupport.cap, params_.window);

  auto swapchainCreateInfo =
      util::generate_swapchain_create_info(data_.surface,
                                           surfaceFormat,
                                           params_.image_count,
                                           data_.extent,
                                           presentMode,
                                           data_.graphics_family,
                                           data_.present_family,
                                           swapchainSupport.cap);

  return vkCreateSwapchainKHR(data_.logical_device,
                              &swapchainCreateInfo,
                              nullptr,
                              &data_.swap_chain) == VK_SUCCESS &&
         data_.swap_chain != nullptr;
}

bool VulkanWrapper::make_swapchain_image_views() {
  auto swapchainImages = std::vector<VkImage>(params_.image_count);

  uint32_t imageCount = params_.image_count;
  vkGetSwapchainImagesKHR(data_.logical_device,
                          data_.swap_chain,
                          &imageCount,
                          swapchainImages.data());

  if (swapchainImages.size() != params_.image_count) return false;

  for (auto image : swapchainImages) {
    auto viewInfo =
        VkImageViewCreateInfo{.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                              .image = image,
                              .viewType = VK_IMAGE_VIEW_TYPE_2D,
                              .format = params_.format,
                              .components{
                                  .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                                  .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                                  .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                                  .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                              },
                              .subresourceRange{
                                  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                  .baseMipLevel = 0,
                                  .levelCount = 1,
                                  .baseArrayLayer = 0,
                                  .layerCount = 1,
                              }};

    VkImageView imageView;
    vkCreateImageView(data_.logical_device, &viewInfo, nullptr, &imageView);

    data_.swap_chain_image_views.push_back(imageView);
  }

  if (data_.swap_chain_image_views.size() != params_.image_count) return false;

  return true;
}

bool VulkanWrapper::make_render_pass() {
  // ? Use ?
  auto colorAttachment = VkAttachmentDescription{
      .format = params_.format,
      .samples = VK_SAMPLE_COUNT_1_BIT,
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

  return vkCreateRenderPass(data_.logical_device,
                            &renderPassInfo,
                            nullptr,
                            &data_.render_pass) == VK_SUCCESS &&
         data_.render_pass != nullptr;
}

bool VulkanWrapper::make_frame_buffers() {
  data_.swap_chain_frame_buffers.resize(data_.swap_chain_image_views.size());

  for (size_t i = 0; i < data_.swap_chain_image_views.size(); ++i) {
    VkImageView attachments[] = {data_.swap_chain_image_views[i]};

    auto frameBufferInfo = VkFramebufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = data_.render_pass,
        .attachmentCount = 1,
        .pAttachments = attachments,
        .width = data_.extent.width,
        .height = data_.extent.height,
        .layers = 1,
    };

    if (vkCreateFramebuffer(data_.logical_device,
                            &frameBufferInfo,
                            nullptr,
                            &data_.swap_chain_frame_buffers[i]) != VK_SUCCESS)
      return false;
  }

  return true;
}

bool VulkanWrapper::make_command_pool() {
  auto poolInfo = VkCommandPoolCreateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = data_.graphics_family,
  };

  return vkCreateCommandPool(data_.logical_device,
                             &poolInfo,
                             nullptr,
                             &data_.command_pool) == VK_SUCCESS &&
         data_.command_pool != nullptr;
}

bool VulkanWrapper::make_command_buffers() {
  data_.command_buffers.resize(data_.swap_chain_frame_buffers.size());

  auto allocInfo = VkCommandBufferAllocateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = data_.command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = static_cast<uint32_t>(data_.command_buffers.size()),
  };

  return vkAllocateCommandBuffers(data_.logical_device,
                                  &allocInfo,
                                  data_.command_buffers.data()) == VK_SUCCESS;
}

bool VulkanWrapper::load_shader() {
  auto vertCode =
      util::readSpvFile(std::string(SHADER_PATH) + "/simple.vert.spv");
  auto fragCode =
      util::readSpvFile(std::string(SHADER_PATH) + "/simple.frag.spv");

  data_.vert_shader_module =
      util::createShaderModule(data_.logical_device, vertCode);
  data_.frag_shader_module =
      util::createShaderModule(data_.logical_device, fragCode);

  return true;
}

bool VulkanWrapper::make_pipeline() {
  auto vertShaderStageInfo = VkPipelineShaderStageCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = data_.vert_shader_module,  // To be set after loading shader
      .pName = "main",
  };

  auto vertexInputInfo = VkPipelineVertexInputStateCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 0,
      .pVertexBindingDescriptions = nullptr,
      .vertexAttributeDescriptionCount = 0,
      .pVertexAttributeDescriptions = nullptr,
  };

  auto fragShaderStageInfo = VkPipelineShaderStageCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = data_.frag_shader_module,  // To be set after loading shader
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
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .frontFace = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1.0f,
  };

  auto multisampling = VkPipelineMultisampleStateCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
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
      .setLayoutCount = 0,
      .pushConstantRangeCount = 0,
  };

  VkPipelineLayout pipelineLayout;
  if (vkCreatePipelineLayout(data_.logical_device,
                             &pipelineLayoutInfo,
                             nullptr,
                             &pipelineLayout) != VK_SUCCESS) {
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

      .layout = pipelineLayout,
      .renderPass = data_.render_pass,
      .subpass = 0,
  };

  return vkCreateGraphicsPipelines(data_.logical_device,
                                   VK_NULL_HANDLE,
                                   1,
                                   &pipelineInfo,
                                   nullptr,
                                   &data_.graphics_pipeline) == VK_SUCCESS &&
         data_.graphics_pipeline != nullptr;
}

bool VulkanWrapper::record_command_buffers() {
  for (size_t i = 0; i < data_.command_buffers.size(); ++i) {
    auto beginInfo = VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    vkBeginCommandBuffer(data_.command_buffers[i], &beginInfo);

    auto clearColor = VkClearValue{
        .color = {{0.1f, 0.1f, 0.15f, 1.0f}},
    };

    auto renderPassInfo = VkRenderPassBeginInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = data_.render_pass,
        .framebuffer = data_.swap_chain_frame_buffers[i],
        .renderArea{
            .offset = {0, 0},
            .extent = data_.extent,
        },
        .clearValueCount = 1,
        .pClearValues = &clearColor,
    };

    vkCmdBeginRenderPass(data_.command_buffers[i],
                         &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(data_.command_buffers[i],
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      data_.graphics_pipeline);

    vkCmdDraw(data_.command_buffers[i], 3, 1, 0, 0);

    vkCmdEndRenderPass(data_.command_buffers[i]);

    if (vkEndCommandBuffer(data_.command_buffers[i]) != VK_SUCCESS) {
      return false;
    }
  }

  return true;
}

bool VulkanWrapper::init_sync() {
  auto semaphoreInfo = VkSemaphoreCreateInfo{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  return vkCreateSemaphore(data_.logical_device,
                           &semaphoreInfo,
                           nullptr,
                           &data_.image_available_semaphore) == VK_SUCCESS &&
         vkCreateSemaphore(data_.logical_device,
                           &semaphoreInfo,
                           nullptr,
                           &data_.render_finished_semaphore) == VK_SUCCESS;
}

void VulkanWrapper::run() {
  uint32_t imageIndex;

  vkAcquireNextImageKHR(data_.logical_device,
                        data_.swap_chain,
                        UINT64_MAX,
                        data_.image_available_semaphore,
                        VK_NULL_HANDLE,
                        &imageIndex);

  VkSemaphore waitSemaphores[] = {data_.image_available_semaphore};

  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSemaphore signalSemaphores[] = {data_.render_finished_semaphore};

  auto submitInfo = VkSubmitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = waitSemaphores,
      .pWaitDstStageMask = waitStages,

      .commandBufferCount = 1,
      .pCommandBuffers = &data_.command_buffers[imageIndex],

      .signalSemaphoreCount = 1,
      .pSignalSemaphores = signalSemaphores,
  };

  vkQueueSubmit(data_.graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);

  VkSwapchainKHR swapchains[] = {data_.swap_chain};

  auto presentInfo = VkPresentInfoKHR{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = signalSemaphores,

      .swapchainCount = 1,
      .pSwapchains = swapchains,
      .pImageIndices = &imageIndex,
  };

  vkQueuePresentKHR(data_.present_queue, &presentInfo);
}