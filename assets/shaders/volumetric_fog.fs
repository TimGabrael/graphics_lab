#version 420 core

in vec3 fragPos;
in vec3 dir;
out vec4 outColor;

uniform mat4 invProjMatrix;
uniform mat4 invViewMatrix;
uniform vec2 screen_size;

uniform sampler2D depth_map;

layout(std140, binding = 0) uniform FogData {
    vec4 center_and_radius; // vec3 center, float radius (w)
    vec4 color;
};
vec3 GetWorldPos(float depth) {
    vec2 screen_pos = 2.0 * gl_FragCoord.xy / screen_size - vec2(1.0, 1.0);
    vec4 clip_space = vec4(screen_pos, depth, 1.0);
    vec4 view_dir_proj = invProjMatrix * clip_space;
    view_dir_proj = view_dir_proj / view_dir_proj.w;
    view_dir_proj.w = 1.0;
    view_dir_proj = invViewMatrix * view_dir_proj;
    return (view_dir_proj / view_dir_proj.w).xyz;
}

float GetFogDepth(vec3 pos, vec3 ndir) {
    vec2 screen_pos = gl_FragCoord.xy / screen_size;
    float depth_buffer_val = texture(depth_map, screen_pos).x;
    vec3 end_world_pos = GetWorldPos(depth_buffer_val * 2.0 - 1.0);
    float max_value = length(end_world_pos - pos);

    vec3 to_pos = pos - center_and_radius.xyz;
    float b = 2.0 * dot(to_pos, ndir);
    float c = dot(to_pos, to_pos) - center_and_radius.w * center_and_radius.w;
    float discriminant = b * b - 4 * c;
    if(discriminant < 0.0) {
        return 0.0;
    }
    float t1 = (-b - sqrt(discriminant)) / 2.0;
    float t2 = (-b + sqrt(discriminant)) / 2.0;
    if(t1 < 0.0 && t2 < 0.0) {
        return 0.0;
    }

    t1 = max(t1, 0.0);
    t2 = max(t2, 0.0);
    t1 = min(t1, max_value);
    t2 = min(t2, max_value);
    return abs(t2 - t1) / (2.0 * center_and_radius.w);
}

void main() {
    vec3 ndir = normalize(dir);
    float interpolation = GetFogDepth(fragPos, ndir);

    outColor = vec4(color.x, color.y, color.z, interpolation);
}

