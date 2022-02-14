#version 450 core

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;
layout (location = 3) in vec4 VertexTangent;

out vec3 LightDir;
out vec2 TexCoord;
out vec3 ViewDir;

#include <../uniforms.glsl>

void main()
{
    // Transform normal and tangent to eye space
    vec3 norm = normalize(vec3(normalMatrix * vec4(VertexNormal, 1.0)));
    vec3 tang = normalize(vec3(normalMatrix * VertexTangent));
    // Compute the binormal
    vec3 binormal = normalize(cross(norm, tang)) * VertexTangent.w;

    // Matrix for transformation to tangent space
    mat3 toObjectLocal = mat3(
        tang.x, binormal.x, norm.x,     // first colum
        tang.y, binormal.y, norm.y,     // second col
        tang.z, binormal.z, norm.z) ;

    // Transform light direction and view direction to tangent space
    vec3 pos = vec3(modelViewMatrix * vec4(VertexPosition, 1.0));
    LightDir =  toObjectLocal * (vec3(1.0, .0, 1.0));
    ViewDir = toObjectLocal * normalize(-pos);

    TexCoord = VertexTexCoord;

    gl_Position = MVP * vec4(VertexPosition, 1.0);
}
