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

 private:
  B backend_;
};