#version 450 core

#include <uniforms.glsl>

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;
layout (location = 3) in vec3 VertexTangent;

// in camera coordinates
out vec3 Position_Geo_In;
out vec3 Normal_Geo_In;
out vec2 TexCoord_Geo_In;
out vec3 Tangent_Geo_In;





void main() {
    vec3 Normal = normalize(VertexNormal);
    Normal_Geo_In = normalize(vec3(normalMatrix * vec4(Normal, 0.0))); 
    Tangent_Geo_In = normalize(vec3(normalMatrix * vec4(VertexTangent, 0.0)));
    TexCoord_Geo_In = VertexTexCoord; 



    vec3 p = VertexPosition;
    if (gui.activeBumpMap && material.hasBump) {
        float t = 2 * texture(bumpMap, VertexTexCoord).r - 1;
        // height update range
        float dist = distance(gui.bbox[0], gui.bbox[1]);
        float HeightRange = dist > 10 ? 1 : 0.07 * dist;
        p +=  t * HeightRange * Normal;
    }

    Position_Geo_In = vec3(modelViewMatrix * vec4(p, 1.0));
    gl_Position = MVP * vec4(p, 1.0);
}

