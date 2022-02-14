#version 450 core

in vec3 LightDir;
in vec2 TexCoord;
in vec3 ViewDir;

#include <../uniforms.glsl>

layout(location = 0) out vec4 FragColor;

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
    // Lookup the normal from the normal map
    vec3 normal = 2.0 * texture(normalMap, TexCoord).xyz - 1.0;
    normal = normalize(normal);
    FragColor = vec4(blinnPhong(normal), 1.0);

}
