#version 450 core

#include <uniforms.glsl>

in vec3 dir;
out vec4 FragColor;

void main() {    
    FragColor = texture(CubeMap, dir);
}