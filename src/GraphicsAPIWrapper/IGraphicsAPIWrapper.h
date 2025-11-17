#pragma once

#include <concepts>
template <typename T>
concept IGraphicsAPIWrapper = requires(T backend) {
  typename T::Data;
  typename T::Params;
  { backend.make_instance() } -> std::same_as<bool>;
  { backend.make_surface() } -> std::same_as<bool>;
  { backend.make_logical_device() } -> std::same_as<bool>;
  { backend.make_swapchain() } -> std::same_as<bool>;
  { backend.make_swapchain_image_views() } -> std::same_as<bool>;
  { backend.make_render_pass() } -> std::same_as<bool>;
};