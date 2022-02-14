#version 450 core

in vec3 Position;
in vec3 Normal;
in vec2 TexCoord;

#include <uniforms.glsl>

layout(location = 0) out vec4 FragColor;


vec3 LightDir = vec3(1, 1, 1);
vec3 ViewDir = -Position;

vec3 blinnPhong(vec3 n) {
  vec3 texColor = texture(diffuseMap, TexCoord).rgb;

  vec3 ambient = 0.1 * texColor;

  vec3 s = normalize(LightDir);
  float sDotN = max(dot(s, n), 0.0);
  vec3 diffuse = texColor * sDotN;
  
  vec3 spec = vec3(0.0);
  if(sDotN > 0.0) {
    vec3 v = normalize(ViewDir);
    vec3 h = normalize(v + s);
    spec = vec3(1.0f, 1.0f, 1.0f) *
            pow(max(dot(h, n), 0.0), 128);
  }
  return ambient + 1.0 * (diffuse + 0.8* spec);
}

void main() {
    // https://stackoverflow.com/questions/5255806/how-to-calculate-tangent-and-binormal
    // compute derivations of the world position
    vec3 p_dx = dFdx(Position);
    vec3 p_dy = dFdy(Position);
    // compute derivations of the texture coordinate
    vec2 tc_dx = dFdx(TexCoord);
    vec2 tc_dy = dFdy(TexCoord);
    // compute initial tangent and bi-tangent
    vec3 Tangent = normalize(tc_dy.y * p_dx - tc_dx.y * p_dy);
    // vec3 BiTangent = normalize(tc_dy.x * p_dx - tc_dx.x * p_dy); // sign inversion

    vec3 N = normalize(Normal);
    vec3 T = normalize(Tangent - N * dot(Tangent, N));
    vec3 B = normalize(cross(N, T)); 
    mat3 TBN = mat3(T, B, N); // col vector as T, B, N.

    // Lookup the normal from the normal map
    vec3 normal = 2.0 * texture(normalMap, TexCoord).xyz - 1.0;
    normal = normalize(TBN * normal);
    if(gl_FrontFacing) 
      FragColor = vec4(blinnPhong(normal), 1.0);
    else 
      FragColor = vec4(blinnPhong(-normal), 1.0);
}
