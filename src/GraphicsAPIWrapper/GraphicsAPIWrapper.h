#pragma once

#include "IGraphicsAPIWrapper.h"
template <IGraphicsAPIWrapper B>
class GraphicsAPIWrapper {
 public:
  template <typename... Args>
  GraphicsAPIWrapper(Args&&... args) : backend_(std::forward<Args>(args)...) {}

  ~GraphicsAPIWrapper() = default;

  bool make_instance() { return backend_.make_instance(); }
  bool make_surface() { return backend_.make_surface(); }
  bool make_logical_device() { return backend_.make_logical_device(); }
  bool make_swapchain() { return backend_.make_swapchain(); }
  bool make_swapchain_image_views() {
    return backend_.make_swapchain_image_views();
  }
  bool make_render_pass() { return backend_.make_render_pass(); }
  bool make_frame_buffers() { return backend_.make_frame_buffers(); }

  bool make_command_pool() { return backend_.make_command_pool(); }
  bool make_command_buffers() { return backend_.make_command_buffers(); }

  bool load_shader() { return backend_.load_shader(); }
  bool make_pipeline() { return backend_.make_pipeline(); }

  bool record_command_buffers() { return backend_.record_command_buffers(); }

  bool init_sync() { return backend_.init_sync(); }
  void run() { return backend_.run(); }

 private:
  B backend_;
};