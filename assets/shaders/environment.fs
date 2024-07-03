#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D hdrEnvMap;
uniform mat4 invViewMatrix;

void main() {
    vec2 ndc = TexCoords * 2.0 - 1.0;
    
    vec3 dir = normalize(vec3(ndc, 1.0));
    
    vec4 worldDir = invViewMatrix * vec4(-dir.x, -dir.y, dir.z, 0.0);
    dir = normalize(worldDir.xyz);
    
    float phi = atan(dir.z, dir.x);
    float theta = -acos(dir.y);
    
    float u = (phi + 3.14159265359) / (2.0 * 3.14159265359);
    float v = theta / 3.14159265359;
    
    vec4 hdrColor = texture(hdrEnvMap, vec2(1.0 - u, v));
    
    FragColor = hdrColor;
}
