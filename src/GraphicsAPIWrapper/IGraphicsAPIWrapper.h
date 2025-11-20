#pragma once

#include <concepts>
template <typename T>
concept IGraphicsAPIWrapper = requires(T backend) {
  typename T::Data;
  typename T::Params;

  { backend.init() } -> std::same_as<void>;
  { backend.run() } -> std::same_as<void>;
  { backend.clean_up() } -> std::same_as<void>;
};