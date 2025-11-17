#include "VulkanWrapper.h"
#include "vulkan_util.h"
#include <cstdint>
#include <vector>

VulkanWrapper::VulkanWrapper(Params &&params, Data &&data) {
  this->params_ = std::make_unique<Params>(std::move(params));
  this->data_ = std::make_unique<Data>(std::move(data));
}

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
  return vkCreateInstance(&createInfo, nullptr, &this->data_->instance) ==
             VK_SUCCESS &&
         this->data_->instance != nullptr;
}

bool VulkanWrapper::make_surface() {
  return glfwCreateWindowSurface(this->data_->instance, this->params_->window,
                                 nullptr,
                                 &this->data_->surface) == VK_SUCCESS &&
         this->data_->surface != nullptr;
}

bool VulkanWrapper::make_logical_device() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(this->data_->instance, &deviceCount, nullptr);
  if (deviceCount == 0) {
    return false;
  }

  auto physicalDevices = std::vector<VkPhysicalDevice>(deviceCount);
  vkEnumeratePhysicalDevices(this->data_->instance, &deviceCount,
                             physicalDevices.data());
  this->data_->physical_device = physicalDevices[this->params_->target_gpu];

  return util::createVkGPU(
             this->data_->physical_device, this->data_->surface,
             &this->data_->graphics_family, &this->data_->present_family,
             &this->data_->logical_device, &this->data_->graphics_queue,
             &this->data_->present_queue) &&
         this->data_->logical_device != nullptr;
}

bool VulkanWrapper::make_swapchain() {
  auto swapchainSupport = util::querySwapchainSupport(
      this->data_->physical_device, this->data_->surface);

  auto surfaceFormat = util::chooseSwapSurfaceFormat(swapchainSupport.formats);
  auto presentMode = util::chooseSwapPresentMode(swapchainSupport.presentModes);
  auto extent =
      util::chooseSwapExtent(swapchainSupport.cap, this->params_->window);

  auto imageCount = (uint32_t)3;

  auto swapchainCreateInfo = util::generate_swapchain_create_info(
      this->data_->surface, surfaceFormat, imageCount, extent, presentMode,
      this->data_->graphics_family, this->data_->present_family,
      swapchainSupport.cap);

  return vkCreateSwapchainKHR(this->data_->logical_device, &swapchainCreateInfo,
                              nullptr,
                              &this->data_->swap_chain) == VK_SUCCESS &&
         this->data_->swap_chain != nullptr;
}