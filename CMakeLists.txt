cmake_minimum_required(VERSION 3.21)
project(vulkanDemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(vulkanDemo
    src/main.cpp
)

target_include_directories(vulkanDemo PRIVATE
    ${VULKAN_SDK}/Include
    src/util
    src/WindowConfig
    src/GraphicsAPIWrapper
)

target_link_libraries(vulkanDemo PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
)

target_sources(vulkanDemo PRIVATE
    src/WindowConfig/GlfwWindowConfig.cpp
    src/GraphicsAPIWrapper/VulkanWrapper.cpp
)
