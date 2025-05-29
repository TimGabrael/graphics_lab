#version 460 core

in vec3 fragPos;
in vec3 fragNor;
in vec2 fragUv;
in vec4 fragCol;

out vec4 outColor;
uniform sampler2D color_map;


layout(std430, binding = 1) buffer SHBuffer {
    vec4 sh_coeffs[9]; // SH L2 (9 bands)
};

vec3 sampleSH(vec3 dir) {
    float x = dir.x, y = dir.y, z = dir.z;

    float sh_basis[9] = float[](
            0.282095,
            0.488603 * y,
            0.488603 * z,
            0.488603 * x,
            1.092548 * x * y,
            1.092548 * y * z,
            0.315392 * (3.0 * z * z - 1.0),
            1.092548 * x * z,
            0.546274 * (x * x - y * y)
            );

    vec3 result = vec3(0.0);
    for (int i = 0; i < 9; ++i) {
        result += sh_coeffs[i].xyz * sh_basis[i];
    }
    return result;
}



void main() {
    vec3 normal = normalize(fragNor);
    vec3 lighting = sampleSH(normal);
    vec4 base_color = texture(color_map, fragUv) * fragCol;
    vec3 final_col = lighting * base_color.xyz;
    outColor = vec4(final_col, base_color.w);
}
