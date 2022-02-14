
#ifndef _UNIFORMS_GLSL_
#define _UNIFORMS_GLSL_

layout(std140, binding = 0) uniform perFrameData {
    uniform mat4 modelViewMatrix;
    uniform mat4 projectionMatrix;
    uniform mat4 normalMatrix;
    uniform mat4 viewportMatrix;
    uniform mat4 MVP;
    uniform vec4 cameraPos;
};

layout(std140, binding = 1) uniform  Material {
    vec4    ambient;
    vec4    diffuse;
    vec4    specular;    
    float   shininess;

    bool    hasTexture;
    bool    hasDiffuse;
    bool    hasAmbient;
    bool    hasNormal;
    bool    hasDisp;
    bool    hasBump;
    bool    hasSpecular;
    bool    hasHeight;
    bool    hasAO;
    bool    hasSkybox;
} material; 

layout(std140, binding = 2) uniform GuiCtrl {
    vec4    bbox[2];
    bool    showBbox;
    bool    showAmbient;
    bool    showDiffuse;
    bool    showSpec;

    bool    activeColorMap;
    bool    activeAmbientMap;
    bool    activeNormalMap;
    bool    activeDispMap;
    bool    activeBumpMap;
    bool    activeSpecMap;
    bool    activeHeightMap;
    bool    activeAoMap;
    bool    activeCubeMap;

    int     showNormal;      /* 0:None  1:Normal  2:Tangent 3:bbox*/
    bool    showWireframe;  /* Drawing a wireframe on top of a shaded mesh */
    bool    polygonMode;    /* 0:Fill  1:Line */
    bool    cullFace;       /* Culls only the back faces */
} gui;


layout(binding = 0) uniform sampler2D diffuseMap;
layout(binding = 1) uniform sampler2D ambientMap;
layout(binding = 2) uniform sampler2D normalMap;
layout(binding = 3) uniform sampler2D dispMap;
layout(binding = 4) uniform sampler2D bumpMap;
layout(binding = 5) uniform sampler2D specularMap;
layout(binding = 6) uniform sampler2D heightMap;
layout(binding = 7) uniform sampler2D AOMap;
layout(binding = 8) uniform samplerCube CubeMap;



#endif 
