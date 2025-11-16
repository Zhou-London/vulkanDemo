#include <stdexcept>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdint>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>

#include "util/vulkan_util.h"
#include "window/GlfwWindowConfig.h"

int main() {
  GlfwWindowConfig glfw_window_config;

  if (!glfw_window_config.init() ||
      !glfw_window_config.createWindow(800, 600, "Vulkan Demo"))
    return -1;

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

  VkInstance instance;
  {
    auto result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS) {
      std::cerr << "Failed creating instance: " << result << "\n";
      return -1;
    }
  }
  VkSurfaceKHR surface;
  {
    auto result = glfwCreateWindowSurface(
        instance, glfw_window_config.getWindow(), nullptr, &surface);
    if (result != VK_SUCCESS) {
      std::cerr << "Failed creating window surface: " << result << "\n";
      return -1;
    }
  }

  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  if (deviceCount == 0) {
    std::cerr << "No devices/GPUs found\n";
    return -1;
  }

  VkDevice vkDevice;
  auto devices = std::vector<VkPhysicalDevice>(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
  uint32_t targetDevice = 0;
  uint32_t graphicsFamily = UINT32_MAX, presentFamily = UINT32_MAX;
  VkQueue graphicsQueue, presentQueue;
  {
    auto result = util::createVkGPU(devices[targetDevice], surface,
                                    &graphicsFamily, &presentFamily, &vkDevice,
                                    &graphicsQueue, &presentQueue);
    if (!result) {
      std::cerr << "Failed creating GPU\n";
      return -1;
    } else {
      std::cout << "Created GPU: " << targetDevice << "\n";
    }
  }

  auto swapchainSupport =
      util::querySwapchainSupport(devices[targetDevice], surface);

  auto surfaceFormat = util::chooseSwapSurfaceFormat(swapchainSupport.formats);
  auto presentMode = util::chooseSwapPresentMode(swapchainSupport.presentModes);
  auto extent = util::chooseSwapExtent(swapchainSupport.cap,
                                       glfw_window_config.getWindow());
  std::cout << "Extent: " << extent.width << "x" << extent.height << "\n";
  auto imageCount = (uint32_t)3;

  auto swapchainCreateInfo = util::generate_swapchain_create_info(
      surface, surfaceFormat, imageCount, extent, presentMode, graphicsFamily,
      presentFamily, swapchainSupport.cap);

  VkSwapchainKHR swapchain;
  if (vkCreateSwapchainKHR(vkDevice, &swapchainCreateInfo, nullptr,
                           &swapchain) != VK_SUCCESS) {
    throw std::runtime_error("Failed creating swapchain");
  }

  while (!glfw_window_config.shouldClose()) {
    glfw_window_config.pollEvents();
  }

  return 0;
}