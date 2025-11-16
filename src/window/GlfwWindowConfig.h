#pragma once
#include <GLFW/glfw3.h>

class GlfwWindowConfig
{
  public:
    GlfwWindowConfig();
    ~GlfwWindowConfig();

    bool init();
    bool createWindow(uint32_t width, uint32_t height, const char *title);
    bool shouldClose() const;
    void pollEvents() const;
    void closeWindow();

    GLFWwindow *getWindow() const;

  private:
    GLFWwindow *window;
};