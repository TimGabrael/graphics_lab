#version 330 core

in vec3 fragPos;
in vec3 fragNor;
in vec2 fragUv;
in vec4 fragCol;

out vec4 outColor;
uniform sampler2D noise_map;
uniform sampler2D depth_map;
uniform vec2 screenSize;

layout(std140) uniform Data {
    vec4 intersection_col;
    vec3 center;
    float max_extent;
    float radius_start;
    float circle_thickness;
    float epsilon;
    float timer;
};

float linearize_depth(float d, float near, float far) {
    return near * far / (far + d *(near - far));
}

void main() {
    vec2 screenPos = gl_FragCoord.xy;
    vec2 uv = screenPos / screenSize;
    float sampled_depth = texture(depth_map, uv).r;
    float current_depth = gl_FragCoord.z;
    
    vec2 delta_noise = vec2(timer / 10.0, timer / 12.3);
    float noise = texture(noise_map, fragUv + delta_noise).r;
    
    float dt = mod(timer, 2.0 * max_extent);

    float radius = 0.0f;
    if(dt > max_extent) {
        radius = 2.0 * (2.0 * max_extent - dt) / max_extent;
    }
    else {
        radius = 2.0 * dt / max_extent;
    }
    float distance = sqrt(dot(fragPos - center, fragPos - center));
    float range = abs(distance - (radius_start + radius));
    float col_multiplier = 0.1f;
    if(range < circle_thickness) {
        col_multiplier = max(range / circle_thickness, col_multiplier);
    }

    sampled_depth = linearize_depth(sampled_depth, 0.01, 100.0);
    current_depth = linearize_depth(current_depth, 0.01, 100.0);
    float diff = abs(current_depth - sampled_depth);
    vec4 col_mul = vec4(col_multiplier, col_multiplier, col_multiplier, 1.0);
    outColor = col_mul * fragCol;
    if(diff < epsilon) {
        outColor = mix(intersection_col, outColor, diff / epsilon);
    }
}

