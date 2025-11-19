#ifndef WINDOWER_H
#define WINDOWER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "renderer.hpp"
#include "ui_panel.hpp"

class Windower {
private:
    GLFWwindow* m_window;

    Renderer& m_renderer;

    int m_windowWidth;
    int m_windowHeight;

public:

    UIPanel panel;
    
    float m_canvasScale = 1.0f;
  
    bool m_middleDown;
    double m_lastMouseX;
    double m_lastMouseY;
    float m_camOX;
    float m_camOY;
    float m_camScale;

    float m_mapMidX = 0.0f;
    float m_mapMidY = 0.0f;
    float m_mapScale = 1.0f;

    void setMapBounds(float midX, float midY, float scale) {
        m_mapMidX = midX;
        m_mapMidY = midY;
        m_mapScale = scale;
    }

    static void m_framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void m_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void m_cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void m_scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    void processInput();
    void handleMouseClick(int button, int action, double xpos, double ypos);
    void resizeViewport(GLFWwindow* window, int width, int height);

    Windower(Renderer& renderer, int windowWidth, int windowHeight);
    ~Windower();
    void run();
};

#endif
