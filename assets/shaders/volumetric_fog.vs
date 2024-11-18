#version 420 core


layout(location = 0) in vec2 aPos;

out vec3 fragPos;
out vec3 dir;

uniform mat4 invProjMatrix;
uniform mat4 invViewMatrix;
uniform vec2 screen_size;

vec3 GetWorldPos(float depth) {
    vec4 clip_space = vec4(aPos, depth, 1.0);
    vec4 view_dir_proj = invProjMatrix * clip_space;
    view_dir_proj = view_dir_proj / view_dir_proj.w;
    view_dir_proj.w = 1.0;
    view_dir_proj = invViewMatrix * view_dir_proj;
    return (view_dir_proj / view_dir_proj.w).xyz;
}

void main() {
    vec3 near_pos = GetWorldPos(0.0);
    vec3 far_pos = GetWorldPos(1.0);

    fragPos = near_pos;
    dir = normalize(far_pos - near_pos);
    gl_Position = vec4(aPos, 0.0, 1.0);
}

