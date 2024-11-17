#version 420 core


layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNor;
layout(location = 2) in vec2 inUv;
layout(location = 3) in vec4 inCol;

out vec3 fragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


void main() {
    fragPos = vec3(model * vec4(inPos, 1.0));
    gl_Position = projection * view * model * vec4(inPos, 1.0);
}
