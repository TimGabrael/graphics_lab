#version 420 core

in vec3 fragPos;

out vec4 outColor;

void main() {
    vec4 colors[8];
    colors[0] = vec4(1.0, 1.0, 1.0, 1.0);
    colors[1] = vec4(1.0, 1.0, 0.0, 1.0);
    colors[2] = vec4(1.0, 0.0, 1.0, 1.0);
    colors[3] = vec4(1.0, 0.0, 0.0, 1.0);
    colors[4] = vec4(0.0, 1.0, 1.0, 1.0);
    colors[5] = vec4(0.0, 1.0, 0.0, 1.0);
    colors[6] = vec4(0.0, 0.0, 1.0, 1.0);
    colors[7] = vec4(0.0, 0.0, 0.0, 1.0);

    int color_idx = gl_PrimitiveID % 8;
    outColor = colors[color_idx];
}
