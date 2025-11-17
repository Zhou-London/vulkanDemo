#pragma once

#include "IGraphicsAPIWrapper.h"
template <IGraphicsAPIWrapper B> class GraphicsAPIWrapper {
public:
  template <typename... Args>
  GraphicsAPIWrapper(Args &&...args) : backend_(std::forward<Args>(args)...) {}

  ~GraphicsAPIWrapper() = default;

  bool make_instance() { return backend_.make_instance(); }
  bool make_surface() { return backend_.make_surface(); }
  bool make_logical_device() { return backend_.make_logical_device(); }
  bool make_swapchain() { return backend_.make_swapchain(); }
  bool make_swapchain_image_views() {
    return backend_.make_swapchain_image_views();
  }

  bool make_render_pass() { return backend_.make_render_pass(); }

private:
  B backend_;
};