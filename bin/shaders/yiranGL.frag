#version 450 core

#include <uniforms.glsl>

in vec3 Position_Frag_in;
in vec3 Normal_Frag_in;
in vec2 TexCoord_Frag_in;
in vec3 Tangent_Frag_in;
noperspective in vec3 EdgeDistance_Frag_in;

out vec4 FragColor;

struct Light {
    vec3 Position; // Light position or direction in eye coords.
    vec3 La;       // Ambient light intensity
    vec3 Ld;       // Diffuse light intensity
    vec3 Ls;       // Specular light intensity
};

Light light = {
    vec3(1.0, 1.0, 1.0),  // position
    vec3(0.1, 0.1, 0.1),  // La
    vec3(1.f, 1.f, 1.f),  // Ld
    vec3(1.f, 1.f, 1.f)   // Ls
};

vec3 blinnPhong(vec3 n) {
    vec3 ambient = vec3(material.ambient);
    vec3 texColor = vec3(material.diffuse);

    if (gui.activeAmbientMap && material.hasAmbient)
        ambient = texture(ambientMap, TexCoord_Frag_in).rgb;

    if (gui.activeColorMap && material.hasDiffuse)
        texColor = texture(diffuseMap, TexCoord_Frag_in).rgb;
 
    ambient = light.La * ambient;

    vec3 s = normalize(light.Position);  // lightDir
    float sDotN = max(dot(s, n), 0.0);
    vec3 diffuse = light.Ld * texColor * sDotN;
    
    vec3 specular = vec3(0.0);
    if(sDotN > 0.0) {
        vec3 v = normalize(-Position_Frag_in);  // ViewDir
        vec3 h = normalize(v + s);
        float spec = pow(max(dot(h, n), 0.0), material.shininess);

        if (gui.activeSpecMap && material.hasSpecular)
            specular = light.Ls * texture(specularMap, TexCoord_Frag_in).rgb * spec;
        else 
            specular = light.Ls * vec3(material.specular) * spec;
    }

/// if (!material.hasDiffuse)
/// return texture(diffuseMap, TexCoord_Frag_in).rgb;
/// return light.Ld * sDotN * vec3(material.diffuse) + light.La * vec3(material.ambient);

    if (!gui.showAmbient)
        ambient = vec3(0,0,0);
    if (!gui.showDiffuse)
        diffuse = vec3(0,0,0);
    if (!gui.showSpec)
        specular = vec3(0,0,0);

    return ambient + diffuse + specular;
}

vec3 mapNormal(vec3 N) {
    // https://stackoverflow.com/questions/5255806/how-to-calculate-tangent-and-binormal
    // compute derivations of the world position
    vec3 p_dx = dFdx(Position_Frag_in);
    vec3 p_dy = dFdy(Position_Frag_in);
    // compute derivations of the texture coordinate
    vec2 tc_dx = dFdx(TexCoord_Frag_in);
    vec2 tc_dy = dFdy(TexCoord_Frag_in);
    // compute initial tangent and bi-tangent
    vec3 Tangent = normalize(tc_dy.y * p_dx - tc_dx.y * p_dy);
    // vec3 BiTangent = normalize(tc_dy.x * p_dx - tc_dx.x * p_dy); // sign inversion

    // vec3 N = normalize(Normal_Frag_in);
    vec3 T = normalize(Tangent - N * dot(Tangent, N));
    vec3 B = normalize(cross(N, T)); 
    mat3 TBN = mat3(T, B, N); // col vector as T, B, N.

    // Lookup the normal from the normal map
    vec3 normal = 2.0 * texture(normalMap, TexCoord_Frag_in).xyz - 1.0;
    return normalize(TBN * normal);
}

void main() {

    if (gui.polygonMode) {
        FragColor = vec4(0.8, 0.8, 0.3, 1);
        return;
    }


    vec3 normal = normalize(Normal_Frag_in);
    if (gui.activeNormalMap && material.hasNormal)
        normal = mapNormal(normal);
    
    if(!gl_FrontFacing)
        normal = -normal;

    vec4 blinnPhongResult = vec4(blinnPhong(normal), 1.0);

    if (gui.showWireframe) {
        // Find the smallest distance
        float d = min(min(EdgeDistance_Frag_in.x, EdgeDistance_Frag_in.y), EdgeDistance_Frag_in.z);
        float mixVal = smoothstep(0.5, 1.f, d);
        FragColor = mix(vec4(0.3, 0.3, 0.3, 1), blinnPhongResult, mixVal);
    }
    else {
        FragColor = blinnPhongResult;
    }
}

