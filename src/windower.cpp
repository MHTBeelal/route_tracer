#include <iostream>
#include <cmath>
#include <vector>

#include "ui_panel.hpp"
#include "windower.hpp"
#include "a_star.hpp"


void ApplyModernDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding = 6.0f;

    style.FrameBorderSize = 1.0f;
    style.WindowBorderSize = 1.0f;

    style.ItemSpacing = ImVec2(10, 10);
    style.WindowPadding = ImVec2(12, 12);

    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.25f, 0.25f, 0.35f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.35f, 0.45f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.45f, 0.45f, 0.55f, 1.0f);

    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.30f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.30f, 0.40f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.40f, 0.50f, 1.0f);

    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.20f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.24f, 0.28f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.32f, 1.0f);
}


Windower::Windower(Renderer& renderer, int windowWidth, int windowHeight)
    : m_renderer(renderer), m_windowWidth(windowWidth), m_windowHeight(windowHeight)
{
    if (!glfwInit()) {
        std::cout << "GLFW not initialized!" << std::endl;
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "Hello World", nullptr, nullptr);
    if (!m_window) {
        std::cout << "Failed to initialize GLFW window" << std::endl;
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(m_window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
    }

    glViewport(0, 0, m_windowWidth, m_windowHeight);
    glfwSetFramebufferSizeCallback(m_window, m_framebufferSizeCallback);

    glfwSetCursorPosCallback(m_window, m_cursorPosCallback);
    glfwSetMouseButtonCallback(m_window, m_mouseButtonCallback);

    glfwSetScrollCallback(m_window, m_scrollCallback);
    glfwSetWindowUserPointer(m_window, this);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ApplyModernDarkTheme();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;     

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);          
    ImGui_ImplOpenGL3_Init();

    m_middleDown = false;
    m_lastMouseX = 0.0;
    m_lastMouseY = 0.0;
    m_camOX = 0.0f;
    m_camOY = 0.0f;
    m_camScale = 1.0f;

    m_renderer.defineGeometry();
    m_renderer.setCamera(m_camOX, m_camOY, m_camScale);
    m_renderer.setViewportSize(m_windowWidth, m_windowHeight);
}

void Windower::run() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        processInput();        

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        panel.ShowUIPanel();

        if (panel.m_runAStarWithNodes) {
            panel.m_runAStarWithNodes = false;
            
            PathResult result = aStarWithNodes(panel.m_startNode, panel.m_endNode);
            if (result.found && !result.nodeIds.empty()) {
                std::cout << "Path found with " << result.nodeIds.size() << " nodes\n";
               
                std::vector<float> pathVertices;
                std::vector<unsigned int> pathIndices;
                
                convertPathToVertices(result.nodeIds, m_mapMidX, m_mapMidY, m_mapScale, pathVertices, pathIndices);
                
                std::cout << "Converted to " << pathVertices.size()/3 << " vertices and " << pathIndices.size() << " indices\n";
                if (!pathVertices.empty() && !pathIndices.empty()) {
                    m_renderer.setPathVertices(pathVertices);
                    m_renderer.setPathIndices(pathIndices);
                } else {
                    std::cout << "ERROR: Path vertices/indices are empty after conversion!\n";
                }
            } else {
                std::cout << "No path found between nodes " << panel.m_startNode << " and " << panel.m_endNode << "\n";
                m_renderer.clearPath();
            }
        }

        if (panel.m_runAStarWithCoords) {
            panel.m_runAStarWithCoords = false;
            
            PathResult result = aStarWithCoords(panel.m_startLat, panel.m_startLon, 
                                               panel.m_endLat, panel.m_endLon);
            if (result.found && !result.nodeIds.empty()) {
                std::cout << "Path found with " << result.nodeIds.size() << " nodes\n";
                std::vector<float> pathVertices;
                std::vector<unsigned int> pathIndices;
                
            
                convertPathToVertices(result.nodeIds, m_mapMidX, m_mapMidY, m_mapScale, pathVertices, pathIndices);
                
                std::cout << "Converted to " << pathVertices.size()/3 << " vertices and " << pathIndices.size() << " indices\n";
                if (!pathVertices.empty() && !pathIndices.empty()) {
                    m_renderer.setPathVertices(pathVertices);
                    m_renderer.setPathIndices(pathIndices);
                } else {
                    std::cout << "ERROR: Path vertices/indices are empty after conversion!\n";
                }
            } else {
                std::cout << "No path found between coordinates\n";
              
                m_renderer.clearPath();
            }
        }


        m_renderer.render();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(m_window);
    }
}

void Windower::processInput() {
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(m_window, true);
    }

    m_renderer.setCamera(m_camOX, m_camOY, m_camScale);
}


void Windower::m_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    Windower* win = reinterpret_cast<Windower*>(glfwGetWindowUserPointer(window));
    if (!win) return;

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            win->m_middleDown = true;
            win->m_lastMouseX = x;
            win->m_lastMouseY = y;
        } else if (action == GLFW_RELEASE) {
            win->m_middleDown = false;
        }
    } else {
        win->handleMouseClick(button, action, x, y);
    }
}

void Windower::handleMouseClick(int button, int action, double xpos, double ypos) {
    if (ImGui::GetIO().WantCaptureMouse) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // Convert screen to NDC
        double ndc_x = (2.0 * xpos) / static_cast<double>(m_windowWidth) - 1.0;
        double ndc_y = -((2.0 * ypos) / static_cast<double>(m_windowHeight) - 1.0);

        // Convert NDC to World (Normalized Map Coords)
        // Shader: gl_Position = vec4((pos.x - offset.x) * scale * aspect, (pos.y - offset.y) * scale, 0.0, 1.0);
        // ndc_x = (world_x - ox) * scale * aspect
        // ndc_y = (world_y - oy) * scale
        
        double aspect = static_cast<double>(m_windowHeight) / static_cast<double>(m_windowWidth);
        double world_x = (ndc_x / (m_camScale * aspect)) - m_camOX;
        double world_y = (ndc_y / m_camScale) - m_camOY;

        // Convert World (Normalized) to Mercator
        // nx = (x_merc - midX) * (2.0f / scale);
        // x_merc = nx * (scale / 2.0f) + midX;
        double x_merc = world_x * (m_mapScale / 2.0) + m_mapMidX;
        double y_merc = world_y * (m_mapScale / 2.0) + m_mapMidY;

        // Convert Mercator to Lat/Lon
        // x_merc = lon_rad
        // y_merc = 0.5 * log((1 + sin(lat)) / (1 - sin(lat)))
        const double rad2deg = 180.0 / M_PI;
        double lon = x_merc * rad2deg;
        double lat_rad = 2.0 * std::atan(std::exp(y_merc)) - (M_PI / 2.0);
        double lat = lat_rad * rad2deg;

        // Find nearest node
        int64_t nodeId = findNearestNode(lat, lon);
        if (nodeId != 0) {
            std::cout << "Selected Node: " << nodeId << " at " << lat << ", " << lon << "\n";
            
            if (panel.m_startNode == 0 || (panel.m_startNode != 0 && panel.m_endNode != 0)) {
                panel.m_startNode = nodeId;
                panel.m_endNode = 0;
                panel.m_startLat = static_cast<float>(lat);
                panel.m_startLon = static_cast<float>(lon);
                m_renderer.clearPath();
            } else {
                panel.m_endNode = nodeId;
                panel.m_endLat = static_cast<float>(lat);
                panel.m_endLon = static_cast<float>(lon);
            }

            std::vector<float> points;
            
            auto addPoint = [&](int64_t id) {
                double nLat, nLon;
                if (getNodeCoords(id, nLat, nLon)) {
                    // Convert back to Normalized Map Coords
                    double nLonRad = nLon * (M_PI / 180.0);
                    double nLatRad = nLat * (M_PI / 180.0);
                    double nXMerc = nLonRad;
                    double nYMerc = 0.5 * std::log((1.0 + std::sin(nLatRad)) / (1.0 - std::sin(nLatRad)));
                    
                    float nX = static_cast<float>((nXMerc - m_mapMidX) * (2.0 / m_mapScale));
                    float nY = static_cast<float>((nYMerc - m_mapMidY) * (2.0 / m_mapScale));
                    
                    points.push_back(nX);
                    points.push_back(nY);
                    points.push_back(0.0f);
                }
            };

            if (panel.m_startNode != 0) addPoint(panel.m_startNode);
            if (panel.m_endNode != 0) addPoint(panel.m_endNode);

            m_renderer.setPoints(points);
        }
    }
}

void Windower::m_cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    Windower* win = reinterpret_cast<Windower*>(glfwGetWindowUserPointer(window));
    if (!win) return;
    if (!win->m_middleDown) return;

    double dx = xpos - win->m_lastMouseX;
    double dy = ypos - win->m_lastMouseY;
    win->m_lastMouseX = xpos;
    win->m_lastMouseY = ypos;

    // convert pixel delta to world-space offset (account for aspect compensation)
    float off_dx = static_cast<float>((2.0 * dx) / (static_cast<double>(win->m_windowHeight) * win->m_camScale));
    float off_dy = static_cast<float>((-2.0 * dy) / (static_cast<double>(win->m_windowHeight) * win->m_camScale));

    win->m_camOX += off_dx;
    win->m_camOY += off_dy;

    win->m_renderer.setCamera(win->m_camOX, win->m_camOY, win->m_camScale);
}

void Windower::m_scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    Windower* win = reinterpret_cast<Windower*>(glfwGetWindowUserPointer(window));
    if (!win) return;

    double cx, cy;
    glfwGetCursorPos(window, &cx, &cy);

    // convert to NDC coordinates used by the shader
    double ndc_x = (2.0 * cx) / static_cast<double>(win->m_windowWidth) - 1.0;
    double ndc_y = -((2.0 * cy) / static_cast<double>(win->m_windowHeight) - 1.0);

    double factor = std::pow(1.125, yoffset);
    double oldScale = win->m_camScale;
    double newScale = oldScale * factor;

    
    double aspect = static_cast<double>(win->m_windowHeight) / static_cast<double>(win->m_windowWidth);
    double screennx_world = ndc_x / (oldScale * aspect);
    double screenny_world = ndc_y / oldScale;

    win->m_camOX = static_cast<float>(win->m_camOX + screennx_world * (1.0/newScale - 1.0/oldScale));
    win->m_camOY = static_cast<float>(win->m_camOY + screenny_world * (1.0/newScale - 1.0/oldScale));
    win->m_camScale = static_cast<float>(newScale);

    win->m_renderer.setCamera(win->m_camOX, win->m_camOY, win->m_camScale);
}

void Windower::resizeViewport(GLFWwindow* window, int width, int height) {
    m_windowWidth = width;
    m_windowHeight = height;
    glViewport(0, 0, m_windowWidth, m_windowHeight);
    m_renderer.setViewportSize(m_windowWidth, m_windowHeight);
}

void Windower::m_framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    Windower* win = reinterpret_cast<Windower*>(glfwGetWindowUserPointer(window));
    if (win) win->resizeViewport(window, width, height);
}

Windower::~Windower() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}
