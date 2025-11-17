#shader vertex
#version 330 core
layout (location = 0) in vec3 aPos;
uniform vec2 u_offset;
uniform float u_scale;

void main() {
    vec2 p = (aPos.xy + u_offset) * u_scale;
    gl_Position = vec4(p.x, p.y, aPos.z, 1.0);
}

#shader fragment
#version 330 core
layout (location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(0.91, 0.44, 0.11, 1.0);
}