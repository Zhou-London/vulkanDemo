#pragma once

#include "IGraphicsAPIWrapper.h"
template <IGraphicsAPIWrapper B>
class GraphicsAPIWrapper {
 public:
  template <typename... Args>
  GraphicsAPIWrapper(Args&&... args) : backend_(std::forward<Args>(args)...) {}

  ~GraphicsAPIWrapper() = default;

  bool init() { return backend_.init(); }

  void run() { return backend_.run(); }

  void clean_up() { return backend_.clean_up(); }

 private:
  B backend_;
};