#version 450 core

#include <uniforms.glsl>

layout (triangles) in;
layout (line_strip, max_vertices = 30) out;

in vec3 Position_Geo_In[];
in vec3 Direction_Geo_In[];

float x_min = gui.bbox[0].x;
float y_min = gui.bbox[0].y;
float z_min = gui.bbox[0].z;

float x_max = gui.bbox[1].x;
float y_max = gui.bbox[1].y;
float z_max = gui.bbox[1].z;

/*
        v5----- v4
       /|      /|
      v1------v0|
      | |     | |
      | |v6---|-|v7
      |/      |/
      v2------v3

      v0 : max
      v6 : min
    right handness
*/
vec4 v0 = MVP * vec4(x_max, y_max, z_max, 1.0);
vec4 v1 = MVP * vec4(x_min, y_max, z_max, 1.0);
vec4 v2 = MVP * vec4(x_min, y_min, z_max, 1.0);
vec4 v3 = MVP * vec4(x_max, y_min, z_max, 1.0);

vec4 v4 = MVP * vec4(x_max, y_max, z_min, 1.0);
vec4 v5 = MVP * vec4(x_min, y_max, z_min, 1.0);
vec4 v6 = MVP * vec4(x_min, y_min, z_min, 1.0);
vec4 v7 = MVP * vec4(x_max, y_min, z_min, 1.0);

vec4 Line_Pair[24] = {
    v0, v1,
    v3, v2,
    v7, v6,
    v4, v5,

    v0, v3,
    v1, v2,
    v5, v6,
    v4, v7,

    v0, v4,
    v1, v5,
    v2, v6,
    v3, v7
};

void EmitNormalLines() {
    float Line_Length = 0.03 * distance(gui.bbox[0], gui.bbox[1]);
    for(int i = 0; i < 2; i++) {
        gl_Position = MVP * vec4(Position_Geo_In[i], 1.0);
        EmitVertex();
        gl_Position = MVP * vec4(Position_Geo_In[i] + Line_Length * Direction_Geo_In[i], 1.0);
        EmitVertex();
        EndPrimitive();
    }
}

void EmitBboxLines() {
    for (int i = 0; i < 24; i+=2) {
        gl_Position = Line_Pair[i + 0];
        EmitVertex();

        gl_Position =  Line_Pair[i + 1];
        EmitVertex();
        EndPrimitive();
    }
}

void main() {
    if (gui.showNormal == 1 || gui.showNormal == 2)
        EmitNormalLines();
    if (gui.showBbox)
        EmitBboxLines();
    return;
}



