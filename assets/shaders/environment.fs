#version 330 core

in vec3 dir;
out vec4 FragColor;

uniform sampler2D hdrEnvMap;
uniform mat4 invViewMatrix;

#define PI 3.14159265359


void main() {
    vec3 ndir = normalize(dir);
    float phi = atan(ndir.z, ndir.x);
    float theta = acos(ndir.y);

    float u = (phi + PI) / (2.0 * PI);
    float v = theta / PI;
    
    vec4 hdrColor = texture(hdrEnvMap, vec2(u, v));
    
    FragColor = hdrColor;
}
