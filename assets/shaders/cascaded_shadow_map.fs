#version 330 core

in vec3 fragPos;
in vec3 fragNor;
in vec2 fragUv;
in vec4 fragCol;

out vec4 outColor;
uniform sampler2D color_map;
uniform sampler2D shadow_map;

uniform vec3 light_direction;

uniform mat4 light_projections[4];
uniform vec4 light_projection_bounds[4];
uniform int enable_debug_view;

float GetShadowInfo() {
    for(int i = 0; i < 4; i++) {
        //float bias = 0.005f / 4.0f * i;
        float bias = 0.00f;
        vec4 lightProj = light_projections[i] * vec4(fragPos, 1.0f);
        lightProj /= lightProj.w;
        if(lightProj.x < -1.0f || lightProj.y < -1.0f  || lightProj.z < -1.0f|| lightProj.x >= 1.0f || lightProj.y >= 1.0f || lightProj.z >= 1.0f) {
            continue;
        }
        vec2 start = light_projection_bounds[i].xy;
        vec2 end = light_projection_bounds[i].zw;
        vec2 step = end - start;
        vec2 uv = (lightProj.xy * vec2(0.5f, 0.5f) + vec2(0.5f, 0.5f)) * step + start;
        float cur_z = lightProj.z * 0.5f + 0.5f;

        float val = texture(shadow_map, uv).x;
        if(val < cur_z + bias) {
            return 0.0f;
        }
        else {
            return 1.0f;
        }
    }
    return 1.0f;
}
int GetDebugInfo() {
    for(int i = 0; i < 4; i++) {
        vec4 lightProj = light_projections[i] * vec4(fragPos, 1.0f);
        lightProj /= lightProj.w;
        if(lightProj.x < -1.0f || lightProj.y < -1.0f || lightProj.x > 1.0f || lightProj.y > 1.0f || lightProj.z < -1.0f || lightProj.z > 1.0f) {
            continue;
        }
        return i;
    }
    return 5;
}

void main() {
    vec3 color_data[8];
    color_data[0] = vec3(1.0f, 0.0f, 0.0f);
    color_data[1] = vec3(1.0f, 0.0f, 1.0f);
    color_data[2] = vec3(1.0f, 1.0f, 0.0f);
    color_data[3] = vec3(1.0f, 1.0f, 1.0f);
    color_data[4] = vec3(0.0f, 0.0f, 0.0f);
    color_data[5] = vec3(0.0f, 0.0f, 1.0f);
    color_data[6] = vec3(0.0f, 1.0f, 0.0f);
    color_data[7] = vec3(0.0f, 1.0f, 1.0f);
    float shadow_val = GetShadowInfo();
    float light_contribution = max(dot(-light_direction, fragNor), 0.0);
    vec4 base_color = texture(color_map, fragUv) * fragCol;
    if(enable_debug_view == 1) {
        int debug_idx = GetDebugInfo();
        base_color *= vec4(color_data[debug_idx], 1.0f);
    }
    outColor = vec4(base_color.xyz * shadow_val * light_contribution, base_color.w);
}
