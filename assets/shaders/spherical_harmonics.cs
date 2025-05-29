#version 460
layout (local_size_x = 16, local_size_y = 16) in;

layout(binding = 0) uniform sampler2D face_texture;
layout(binding = 1, std430) buffer SHBuffer {
    vec4 sh_coeffs[9];
};

uniform mat4 inv_view_proj;
uniform int image_width;
uniform int image_height;


shared vec3 localSH[9][16][16];
void main() {
    ivec2 pixel_coord = ivec2(gl_GlobalInvocationID.xy);
    uvec2 lid = gl_LocalInvocationID.xy;
    if (pixel_coord.x >= image_width || pixel_coord.y >= image_height)
        return;

    vec2 uv = (vec2(pixel_coord) + 0.5) / vec2(image_width, image_height);
    vec2 ndc = uv * 2.0 - 1.0;

    vec4 dir4 = inv_view_proj * vec4(ndc.x, ndc.y, 1.0, 1.0);
    vec3 dir = normalize(dir4.xyz / dir4.w);

    vec3 color = texture(face_texture, uv).rgb;

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
    float texelAngle = (4.0 * 3.14159265) / (6.0 * float(image_width * image_height));
    for(int i = 0; i < 9; ++i) {
        localSH[i][lid.x][lid.y] = color * sh_basis[i] * texelAngle * (4.0 * 3.14159265);
    }
    
    barrier();


    // only one thread writes the data into the buffer
    if (lid.x == 0 && lid.y == 0) {
        float convolution_values[9] = float[](
            0.887227,
            1.02333,
            1.02333,
            1.02333,
            0.858086,
            0.858086,
            0.858086,
            0.858086,
            0.858086
        );
        for (int i = 0; i < 9; ++i) {
            vec3 sum = vec3(0.0);
            for (int x = 0; x < 16; ++x)
                for (int y = 0; y < 16; ++y)
                    sum += localSH[i][x][y];

            sh_coeffs[i].xyz += sum * convolution_values[i];
            sh_coeffs[i].w = 0.0;
        }
    }
}
