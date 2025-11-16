#pragma once

#include <cstdint>
#include <set>
#include <vector>
#include <vulkan/vulkan.h>

namespace util
{
inline bool createVkGPU(VkPhysicalDevice device, VkSurfaceKHR *surface, uint32_t *graphicsFamily,
                        uint32_t *presentFamily, VkDevice *vkDevice, VkQueue *graphicsQueue, VkQueue *presentQueue)
{
    *graphicsFamily = UINT32_MAX;
    *presentFamily = UINT32_MAX;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    auto queueFamilies = std::vector<VkQueueFamilyProperties>(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (auto i = 0; i < queueFamilyCount; ++i)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            *graphicsFamily = i;
            break;
        }
    }

    for (auto i = 0; i < queueFamilyCount; ++i)
    {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, *surface, &presentSupport);
        if (presentSupport)
        {
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

    for (auto queueFamily : uniqueQueueFamilies)
    {
        auto queueCreateInfo = VkDeviceQueueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueFamily,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        };
        queueCreateInfos.push_back(queueCreateInfo);
    }

    const auto deviceExtensions = std::vector<const char *>{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    auto createInfo = VkDeviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = (uint32_t)queueCreateInfos.size(),
        .pQueueCreateInfos = queueCreateInfos.data(),

        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,

        .enabledExtensionCount = (uint32_t)deviceExtensions.size(),
        .ppEnabledExtensionNames = deviceExtensions.data(),
    };

    if (vkCreateDevice(device, &createInfo, nullptr, vkDevice) != VK_SUCCESS)
    {
        return false;
    }

    vkGetDeviceQueue(*vkDevice, *graphicsFamily, 0, graphicsQueue);
    vkGetDeviceQueue(*vkDevice, *presentFamily, 0, presentQueue);

    return true;
}
} // namespace util