#ifndef __UINIFORMS_H__
#define __UINIFORMS_H__
#include "mat4.h"


/* sharing of uniform data between programs. */
/* When changing programs, the same buffer object need only be rebound to the corresponding block in the new program. */
/* All programs we set same binding block for same struct, so, we need't rebound */

unsigned int uniform_matrix_handle;
unsigned int uniform_mtl_handle;
unsigned int uniform_gui_ctl_handle;

typedef struct uniform_matrix {
    mat4 modelViewMatrix;
    mat4 projectionMatrix;
    mat4 normalMatrix;
    mat4 viewportMatrix;
    mat4 MVP;
    vec4 cameraPos;
} uniform_matrix;

typedef struct uniform_mtl {
    vec4    ambient;
    vec4    diffuse;
    vec4    specular;    
    float   shininess;

    int    hasTexture;
    int    hasDiffuse;
    int    hasAmbient;
    int    hasNormal;
    int    hasDisp;
    int    hasBump;
    int    hasSpecular;
    int    hasHeight;
    int    hasAO;
    int    hasSkybox;
} uniform_mtl; 

typedef struct uniform_gui_ctl {
    vec4    bbox[2];
    int     showBbox;
    int     showAmbient;
    int     showDiffuse;
    int     showSpec;

    int     activeColorMap;
    int     activeAmbientMap;
    int     activeNormalMap;
    int     activeDispMap;
    int     activeBumpMap;
    int     activeSpecMap;
    int     activeHeightMap;
    int     activeAoMap;
    int     activeCubeMap;

    int     showNormal;     /* 0:None  1:Normal  2:Tangent */
    int     showWireframe;  /* Drawing a wireframe on top of a shaded mesh */
    int     polygonMode;    /* 0:Fill  1:Line */
    int     cullFace;       /* Culls only the back faces */
} uniform_gui_ctl;

#endif 
