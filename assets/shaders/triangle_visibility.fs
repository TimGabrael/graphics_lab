#version 420 core

in vec3 fragPos;

out vec4 outColor;

void main() {
    vec4 colors[27];
    colors[0] = vec4(1.0, 1.0, 1.0, 1.0);
    colors[1] = vec4(1.0, 1.0, 0.5, 1.0);
    colors[2] = vec4(1.0, 1.0, 0.0, 1.0);
    colors[3] = vec4(1.0, 0.5, 1.0, 1.0);
    colors[4] = vec4(1.0, 0.5, 0.5, 1.0);
    colors[5] = vec4(1.0, 0.5, 0.0, 1.0);
    colors[6] = vec4(1.0, 0.0, 1.0, 1.0);
    colors[7] = vec4(1.0, 0.0, 0.5, 1.0);
    colors[8] = vec4(0.5, 0.0, 0.0, 1.0);
    colors[9] = vec4(0.5, 1.0, 1.0, 1.0);
    colors[10] = vec4(0.5, 1.0, 0.5, 1.0);
    colors[11] = vec4(0.5, 1.0, 0.0, 1.0);
    colors[12] = vec4(0.5, 0.5, 1.0, 1.0);
    colors[13] = vec4(0.5, 0.5, 0.5, 1.0);
    colors[14] = vec4(0.5, 0.5, 0.0, 1.0);
    colors[15] = vec4(0.5, 0.0, 1.0, 1.0);
    colors[16] = vec4(0.5, 0.0, 0.5, 1.0);
    colors[17] = vec4(0.0, 0.0, 0.0, 1.0);
    colors[18] = vec4(0.0, 1.0, 1.0, 1.0);
    colors[19] = vec4(0.0, 1.0, 0.5, 1.0);
    colors[20] = vec4(0.0, 1.0, 0.0, 1.0);
    colors[21] = vec4(0.0, 0.5, 1.0, 1.0);
    colors[22] = vec4(0.0, 0.5, 0.5, 1.0);
    colors[23] = vec4(0.0, 0.5, 0.0, 1.0);
    colors[24] = vec4(0.0, 0.0, 1.0, 1.0);
    colors[25] = vec4(0.0, 0.0, 0.5, 1.0);
    colors[26] = vec4(0.0, 0.0, 0.0, 1.0);

    int color_idx = gl_PrimitiveID % 27;
    outColor = colors[color_idx];
}
