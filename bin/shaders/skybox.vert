#version 450 core

#include <uniforms.glsl>

layout (location = 0) in vec3 VertexPosition;

out vec3 dir;

void main() {
    dir = VertexPosition;
    vec4 pos = projectionMatrix * mat4(mat3(modelViewMatrix)) * vec4(VertexPosition, 1.0);
    gl_Position = pos.xyww; 
    // The resulting normalized device coordinates will then 
    // always have a z value equal to 1.0: the maximum depth value.
}  


// remove the translation section of transformation matrices 
// by taking the upper-left 3x3 matrix of the 4x4 matrix.