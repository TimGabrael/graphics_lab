#version 330 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNor;
layout(location = 2) in vec2 inUv;
layout(location = 3) in vec4 inCol;

out vec4 fragCol;

uniform mat4 model;
uniform mat4 view_proj;
uniform vec4 basic_color;

void main() {
    fragCol = basic_color;

    gl_Position = view_proj * model * vec4(inPos, 1.0);
}


