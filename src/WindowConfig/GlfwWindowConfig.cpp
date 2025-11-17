#include "GlfwWindowConfig.h"

GlfwWindowConfig::GlfwWindowConfig() { window = nullptr; }

GlfwWindowConfig::~GlfwWindowConfig() { closeWindow(); }

bool GlfwWindowConfig::init() {
  if (!glfwInit())
    return false;

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  return true;
}

bool GlfwWindowConfig::createWindow(uint32_t width, uint32_t height,
                                    const char *title) {
  window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (!window)
    return false;
  return true;
}

bool GlfwWindowConfig::shouldClose() const {
  return glfwWindowShouldClose(window);
}

void GlfwWindowConfig::pollEvents() const { glfwPollEvents(); }

void GlfwWindowConfig::closeWindow() {
  glfwDestroyWindow(window);
  glfwTerminate();
}

GLFWwindow *GlfwWindowConfig::getWindow() const { return window; }