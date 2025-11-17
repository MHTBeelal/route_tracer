#include "renderer.hpp"

Renderer::Renderer() {
    readShader("res/shaders/basic.shader");
}

void Renderer::render() const
{   
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(m_shaderProgram);
    glBindVertexArray(m_VAO);
    // If segment info is available, draw each segment as a line strip for continuous roads
    if (!m_segmentOffsets.empty() && m_segmentOffsets.size() == m_segmentLengths.size()) {
        for (size_t i = 0; i < m_segmentOffsets.size(); ++i) {
            size_t offset = m_segmentOffsets[i];
            size_t len = m_segmentLengths[i];
            if (len < 2) continue;
            glDrawElements(GL_LINE_STRIP, static_cast<GLsizei>(len), GL_UNSIGNED_INT, reinterpret_cast<const void*>(offset * sizeof(unsigned int)));
        }
    } else {
        glDrawElements(m_drawMode, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
    }
}

void Renderer::defineGeometry() 
{
    GLuint VBO;
    glGenBuffers(1, &VBO);

    glGenVertexArrays(1, &m_VAO);

    GLuint EBO;
    glGenBuffers(1, &EBO);

    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(float), m_vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), m_indices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);


    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);
    glEnableVertexAttribArray(0);


    // Vertex shader
    GLuint vertexShader = createShader(GL_VERTEX_SHADER, m_vertexShaderSource);

    //Fragment shader
    GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, m_fragmentShaderSource);

    //Linking shaders into a program
    GLuint shaderProgram = linkShadersIntoProgram({vertexShader, fragmentShader});
    m_shaderProgram = shaderProgram;

    // default draw mode
    m_drawMode = GL_TRIANGLES;

    // get uniform locations for camera and set defaults
    m_uOffsetLoc = glGetUniformLocation(m_shaderProgram, "u_offset");
    m_uScaleLoc = glGetUniformLocation(m_shaderProgram, "u_scale");
    if (m_uOffsetLoc >= 0) glUniform2f(m_uOffsetLoc, m_camOffsetX, m_camOffsetY);
    if (m_uScaleLoc >= 0) glUniform1f(m_uScaleLoc, m_camScale);

    // make lines more visible
    glLineWidth(1.5f);
}

void Renderer::readShader(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << filepath << std::endl;
    }


    enum ShaderType { NONE = -1, VERTEX = 0, FRAGMENT = 1 };
    ShaderType type = ShaderType::NONE;

    std::string line;
    std::stringstream ss[2];

    while(getline(file, line)) {
        if (line.find("#shader vertex") != std::string::npos) {
            type = ShaderType::VERTEX;
        } else if (line.find("#shader fragment") != std::string::npos) {
            type = ShaderType::FRAGMENT;
        } else if (type != ShaderType::NONE) {
            ss[static_cast<int>(type)] << line << "\n";
        }
    }

    m_vertexShaderSource = ss[0].str();
    m_fragmentShaderSource = ss[1].str();
}

GLuint Renderer::createShader(GLenum type, const std::string &source)
{
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    // Error checking
    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::string log(length, ' ');
        glGetShaderInfoLog(shader, length, &length, &log[0]);
        std::cerr << "Shader compile error: " << log << std::endl;
    }

    return shader;
}

GLuint Renderer::linkShadersIntoProgram(const std::vector<GLuint>&& shaders) {
    GLuint shaderProgram = glCreateProgram();

    for(const auto& i : shaders) 
        glAttachShader(shaderProgram, i);
    
    glLinkProgram(shaderProgram);


    // Error checking
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "Shader Linking Error: " << infoLog << std::endl; 
    }


    return shaderProgram;
}