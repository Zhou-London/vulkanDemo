
#include "util/vulkan_util.h"
#include <cstdint>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>

int main()
{
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    auto window = glfwCreateWindow(800, 600, "VulkanDemo", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

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
        if (result != VK_SUCCESS)
        {
            std::cerr << "Failed creating instance: " << result << "\n";
            return -1;
        }
    }
    VkSurfaceKHR surface;
    {
        auto result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
        if (result != VK_SUCCESS)
        {
            std::cerr << "Failed creating window surface: " << result << "\n";
            return -1;
        }
    }

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
        std::cerr << "No devices/GPUs found\n";
        return -1;
    }

    auto devices = std::vector<VkPhysicalDevice>(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    uint32_t targetDevice = 0;
    uint32_t graphicsFamily = UINT32_MAX, presentFamily = UINT32_MAX;
    VkDevice vkDevice;
    VkQueue graphicsQueue, presentQueue;
    {
        auto result = util::createVkGPU(devices[targetDevice], &surface, &graphicsFamily, &presentFamily, &vkDevice,
                                        &graphicsQueue, &presentQueue);
        if (!result)
        {
            std::cerr << "Failed creating GPU\n";
            return -1;
        }
        else
        {
            std::cout << "Created GPU\n";
        }
    }

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}