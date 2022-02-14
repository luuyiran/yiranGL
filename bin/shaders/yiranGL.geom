#version 450 core

#include <uniforms.glsl>

layout(triangles) in;
layout(triangle_strip, max_vertices = 999) out;

in vec3 Position_Geo_In[];
in vec3 Normal_Geo_In[];
in vec2 TexCoord_Geo_In[];
in vec3 Tangent_Geo_In[];

out vec3 Position_Frag_in;
out vec3 Normal_Frag_in;
out vec2 TexCoord_Frag_in;
out vec3 Tangent_Frag_in;
// noperspective qualifier: interpolated linearly, instead of the default perspective correct interpolation
noperspective out vec3 EdgeDistance_Frag_in;

// Transform each vertex into viewport space
vec2 p0 = vec2(viewportMatrix * (gl_in[0].gl_Position / gl_in[0].gl_Position.w));
vec2 p1 = vec2(viewportMatrix * (gl_in[1].gl_Position / gl_in[1].gl_Position.w));
vec2 p2 = vec2(viewportMatrix * (gl_in[2].gl_Position / gl_in[2].gl_Position.w));

float a = length(p1 - p2);
float b = length(p2 - p0);
float c = length(p1 - p0);
float alpha = acos((b*b + c*c - a*a) / (2.0*b*c));
float beta = acos((a*a + c*c - b*b) / (2.0*a*c));
float ha = abs(c * sin(beta));
float hb = abs(c * sin(alpha));
float hc = abs(b * sin(alpha));
vec3 EdgeDist[3] = {vec3(ha, 0, 0), vec3(0, hb, 0), vec3(0, 0, hc)};


void EmitVertexAt(int i) {
    EdgeDistance_Frag_in = EdgeDist[i];
    TexCoord_Frag_in     = TexCoord_Geo_In[i];
    Normal_Frag_in       = Normal_Geo_In[i];
    Position_Frag_in     = Position_Geo_In[i];
    Tangent_Frag_in      = Tangent_Geo_In[i];
    gl_Position          = gl_in[i].gl_Position;
    EmitVertex();
}

void main() {
    EmitVertexAt(0);
    EmitVertexAt(1);
    EmitVertexAt(2);
    EndPrimitive();
}
