#version 330 core

layout (location = 0) in vec2 aPos;

uniform mat4 invProjMatrix;
uniform mat4 viewMatrix;

out vec3 dir;

void main() {
    vec4 clip_space = vec4(aPos, 1.0, 1.0);
    vec4 view_dir_proj = invProjMatrix * clip_space;
    view_dir_proj /= view_dir_proj.w;
    vec3 view_dir = normalize(view_dir_proj.xyz) * mat3(viewMatrix);
    
    dir = view_dir;

    gl_Position = vec4(aPos, 0.0, 1.0);
}
