#shader vertex
#version 330 core
layout (location = 0) in vec3 aPos;

uniform vec2 u_offset;
uniform float u_scale;
uniform float u_aspect;

void main() {
    vec2 p;
    p.x = (aPos.x + u_offset.x) * u_scale * u_aspect;
    p.y = (aPos.y + u_offset.y) * u_scale;
    gl_Position = vec4(p.x, p.y, aPos.z, 1.0);
}

#shader fragment
#version 330 core
layout (location = 0) out vec4 fragColor;

uniform vec3 u_color;

void main() {
    fragColor = vec4(u_color, 1.0);
}