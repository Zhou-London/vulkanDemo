#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstdint>
#include <set>
#include <vector>

namespace util {
struct SwapchainSupportDetails {
  VkSurfaceCapabilitiesKHR cap;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

inline bool createVkGPU(VkPhysicalDevice device, VkSurfaceKHR surface,
                        uint32_t *graphicsFamily, uint32_t *presentFamily,
                        VkDevice *vkDevice, VkQueue *graphicsQueue,
                        VkQueue *presentQueue) {
  *graphicsFamily = UINT32_MAX;
  *presentFamily = UINT32_MAX;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  auto queueFamilies = std::vector<VkQueueFamilyProperties>(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies.data());

  for (auto i = 0; i < queueFamilyCount; ++i) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      *graphicsFamily = i;
      break;
    }
  }

  for (auto i = 0; i < queueFamilyCount; ++i) {
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
    if (presentSupport) {
      *presentFamily = i;
      break;
    }
  }

  if (*graphicsFamily == UINT32_MAX || *presentFamily == UINT32_MAX)
    return false;

  auto queueCreateInfos = std::vector<VkDeviceQueueCreateInfo>();
  auto uniqueQueueFamilies = std::set<uint32_t>{
      *graphicsFamily,
      *presentFamily,
  };

  float queuePriority = 1.0f;

  for (auto queueFamily : uniqueQueueFamilies) {
    auto queueCreateInfo = VkDeviceQueueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };
    queueCreateInfos.push_back(queueCreateInfo);
  }

  const auto deviceExtensions =
      std::vector<const char *>{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  auto createInfo = VkDeviceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = (uint32_t)queueCreateInfos.size(),
      .pQueueCreateInfos = queueCreateInfos.data(),

      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,

      .enabledExtensionCount = (uint32_t)deviceExtensions.size(),
      .ppEnabledExtensionNames = deviceExtensions.data(),
  };

  if (vkCreateDevice(device, &createInfo, nullptr, vkDevice) != VK_SUCCESS) {
    return false;
  }

  vkGetDeviceQueue(*vkDevice, *graphicsFamily, 0, graphicsQueue);
  vkGetDeviceQueue(*vkDevice, *presentFamily, 0, presentQueue);

  return true;
}

inline SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device,
                                                     VkSurfaceKHR surface) {
  SwapchainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.cap);

  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

  details.formats.resize(formatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                       details.formats.data());

  uint32_t presentModeCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
                                            nullptr);

  details.presentModes.resize(presentModeCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
                                            details.presentModes.data());

  return details;
}

// ! Complete fail condition
inline VkSurfaceFormatKHR
chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats,
                        VkFormat targetFormat) {
  for (const auto &format : availableFormats)
    if (format.format == targetFormat &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      return format;

  return availableFormats[0];
}

inline VkPresentModeKHR chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes, VkPresentModeKHR targetPresentMode) {
  for (const auto &presentMode : availablePresentModes)
    if (presentMode == targetPresentMode)
      return presentMode;

  return VK_PRESENT_MODE_FIFO_KHR;
}

inline VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &cap,
                                   GLFWwindow *window) {
  if (cap.currentExtent.width != UINT32_MAX)
    return cap.currentExtent;

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  auto actualExtent = VkExtent2D{
      .width = static_cast<uint32_t>(width),
      .height = static_cast<uint32_t>(height),
  };

  actualExtent.width = std::clamp(actualExtent.width, cap.minImageExtent.width,
                                  cap.maxImageExtent.width);
  actualExtent.height =
      std::clamp(actualExtent.height, cap.minImageExtent.height,
                 cap.maxImageExtent.height);

  return actualExtent;
}

inline VkSwapchainCreateInfoKHR generate_swapchain_create_info(
    VkSurfaceKHR surface, VkSurfaceFormatKHR surfaceFormat,
    uint32_t imageCount, VkExtent2D extent, VkPresentModeKHR presentMode,
    uint32_t graphicsFamily, uint32_t presentFamily,
    VkSurfaceCapabilitiesKHR &cap) {
  auto createInfo = VkSwapchainCreateInfoKHR{
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = surface,
      .minImageCount = imageCount,
      .imageFormat = surfaceFormat.format,
      .imageColorSpace = surfaceFormat.colorSpace,
      .imageExtent = extent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .preTransform = cap.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = presentMode,
      .clipped = VK_TRUE,
      .oldSwapchain = VK_NULL_HANDLE,
  };

  uint32_t queueFamilyIndices[] = {graphicsFamily, presentFamily};

  if (graphicsFamily != presentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  return createInfo;
}

inline VkImageViewCreateInfo generate_image_view_create_info(VkImage image,
                                                             VkFormat format) {
  auto viewInfo =
      VkImageViewCreateInfo{.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                            .image = image,
                            .viewType = VK_IMAGE_VIEW_TYPE_2D,
                            .format = format,
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

  return viewInfo;
}
} // namespace util