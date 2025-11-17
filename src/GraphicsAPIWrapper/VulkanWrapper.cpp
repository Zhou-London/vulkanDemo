#include "VulkanWrapper.h"
#include "vulkan_util.h"
#include <cstdint>
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
        util::generate_image_view_create_info(image, params_.format);

    VkImageView imageView;
    vkCreateImageView(data_.logical_device, &viewInfo, nullptr, &imageView);

    data_.swap_chain_image_views.push_back(imageView);
  }

  if (data_.swap_chain_image_views.size() != params_.image_count) return false;

  return true;
}

bool VulkanWrapper::make_render_pass() {
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

    auto frameBufferInfo =
        util::generate_frame_buffer_create_info(data_.render_pass,
                                                data_.extent,
                                                attachments);

    if (vkCreateFramebuffer(data_.logical_device,
                            &frameBufferInfo,
                            nullptr,
                            &data_.swap_chain_frame_buffers[i]) != VK_SUCCESS)
      return false;
  }

  return true;
}