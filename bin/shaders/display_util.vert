#version 450 core

#include <uniforms.glsl>

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;
layout (location = 3) in vec3 VertexTangent;

out vec3 Position_Geo_In;
out vec3 Direction_Geo_In;

vec3 MapNormal(vec3 N, vec3 T) {
    vec3 B = normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    vec3 normal = 2.0 * texture(normalMap, VertexTexCoord).xyz - 1.0;
    return normalize(TBN * normal);
}




void main() {
    vec3 Normal = normalize(VertexNormal);
    vec3 N = Normal;
    vec3 T = normalize(VertexTangent);
    if (gui.activeNormalMap && material.hasNormal) {
        N = MapNormal(N, T);
    }

    if (gui.showNormal == 1)
        Direction_Geo_In = N; 
    else if (gui.showNormal == 2)
        Direction_Geo_In = T; // show tangent

    vec3 p = VertexPosition;
    if (gui.activeBumpMap && material.hasBump) {
        float t = 2 * texture(bumpMap, VertexTexCoord).r - 1;
        // height update range
        float dist = distance(gui.bbox[0], gui.bbox[1]);
        float HeightRange = dist > 10 ? 1 : 0.07 * dist;
        p +=  t * HeightRange * Normal;
    }

    Position_Geo_In = p;
}
