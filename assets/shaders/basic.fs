#version 330 core

in vec3 fragPos;
in vec3 fragNor;
in vec2 fragUv;
in vec4 fragCol;

out vec4 outColor;
uniform sampler2D color_map;


void main() {
    outColor = texture(color_map, fragUv) * fragCol;
}
