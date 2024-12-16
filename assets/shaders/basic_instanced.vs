#version 330 core


layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNor;
layout(location = 2) in vec2 inUv;
layout(location = 3) in vec4 inCol;
layout(location = 4) in vec3 instancePos;
layout(location = 5) in vec4 instanceCol;

out vec3 fragPos;
out vec3 fragNor;
out vec2 fragUv;
out vec4 fragCol;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vec3 pos = inPos + instancePos;
    fragPos = vec3(model * vec4(pos, 1.0));
    fragNor = mat3(transpose(inverse(model))) * inNor;
    fragUv = inUv;
    fragCol = inCol * instanceCol;

    gl_Position = projection * view * model * vec4(pos, 1.0);
}

