#ifndef WINDOWER_H
#define WINDOWER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "renderer.hpp"

class Windower {
private:
    GLFWwindow* m_window;
    Renderer& m_renderer;

    int m_windowWidth;
    int m_windowHeight;

    bool m_middleDown;
    double m_lastMouseX;
    double m_lastMouseY;
    float m_camOX;
    float m_camOY;
    float m_camScale;

    static void m_framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void m_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void m_cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void m_scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    void processInput();
    void resizeViewport(GLFWwindow* window, int width, int height);

public:
    Windower(Renderer& renderer, int windowWidth, int windowHeight);
    void run();
    ~Windower();
};

#endif
