#ifndef RENDERER
#define RENDERER

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>



class Renderer {
private:

    std::vector<float> m_vertices;
    std::vector<unsigned int> m_indices;

    GLuint m_VAO;
    GLenum m_drawMode;
    std::vector<size_t> m_segmentOffsets;
    std::vector<size_t> m_segmentLengths;

    std::string m_vertexShaderSource;
    std::string m_fragmentShaderSource;

    GLuint m_shaderProgram;
    GLint m_uOffsetLoc = -1;
    GLint m_uScaleLoc = -1;
    float m_camOffsetX = 0.0f;
    float m_camOffsetY = 0.0f;
    float m_camScale = 1.0f;

    void readShader(const std::string& filepath);
    GLuint createShader(GLenum type, const std::string& source);
    GLuint linkShadersIntoProgram(const std::vector<GLuint>&& shaders);
    

public:

    Renderer();
    ~Renderer() = default;

    void setDrawMode(GLenum mode) { m_drawMode = mode; }

    void setSegmentInfo(const std::vector<size_t>& offsets, const std::vector<size_t>& lengths) {
        m_segmentOffsets = offsets;
        m_segmentLengths = lengths;
    }

    void setCamera(float ox, float oy, float scale) {
        m_camOffsetX = ox;
        m_camOffsetY = oy;
        m_camScale = scale;
        if (m_shaderProgram) {
            glUseProgram(m_shaderProgram);
            if (m_uOffsetLoc >= 0) glUniform2f(m_uOffsetLoc, m_camOffsetX, m_camOffsetY);
            if (m_uScaleLoc >= 0) glUniform1f(m_uScaleLoc, m_camScale);
        }
    }

    void setVertices(const std::vector<float>& arr) { m_vertices = arr; }
    void setIndices(const std::vector<unsigned int>& arr) { m_indices = arr; }

    void render() const;
    void defineGeometry();
};

#endif