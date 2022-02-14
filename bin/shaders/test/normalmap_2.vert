#version 450 core

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;
layout (location = 3) in vec4 VertexTangent;

out vec3 Position;
out vec3 Normal;
out vec2 TexCoord;

#include <uniforms.glsl>

void main() {
    // eye space
    Position = vec3(modelViewMatrix * vec4(VertexPosition, 1.0));
    Normal = vec3(normalMatrix * vec4(VertexNormal, 1.0)); 
    TexCoord = VertexTexCoord; 
    
    vec3 p = VertexPosition;
    if (material.hasBump) {
        float height = 2 * texture(bumpMap, VertexTexCoord).r - 1;
        p +=  height * VertexNormal;
    }
    gl_Position = MVP * vec4(p, 1.0);
}




