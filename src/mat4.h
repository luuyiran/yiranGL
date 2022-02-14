#ifndef __MAT_H__
#define __MAT_H__

#define MAT4_EPSILON  (1e-16f)

typedef struct vec2 { float x, y; } vec2;
typedef struct vec3 { float x, y, z; } vec3;
typedef struct vec4 { float x, y, z, w; } vec4;
typedef struct quat { float x, y, z, w; } quat;
typedef struct mat4 { vec4 col[4]; } mat4;       /* column-major */

#define RADIANS(degrees) ((degrees) * 0.0174532925f)
#define DEGREES(radians) ((radians) * 57.295779513f)

#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef M_PI
#define M_PI       3.14159265358979323846 
#endif 

#ifdef __cplusplus
extern "C" {
#endif

mat4 lookAt(vec3 eye, vec3 center, vec3 up);
mat4 ortho(float fov, float aspect, float n, float f);
mat4 perspective(float fovy, float aspect, float near, float far);
mat4 view_port_matrix(int width, int height);

mat4 mat4_identity(void);
mat4 scale(float x);
mat4 translate(vec3 v);
mat4 rotate(float angle, vec3 axis);
mat4 rotate_x(float angle);
mat4 rotate_y(float angle);
mat4 rotate_z(float angle);

vec3 vec3_normalize(vec3 v);
vec3 vec3_cross(vec3 a, vec3 b);
float vec3_len(vec3 v);
float vec3_dot(vec3 a, vec3 b);
vec3 vec3_mul_float(vec3 v, float f);
vec3 vec3_add(vec3 a, vec3 b);
vec3 vec3_sub(vec3 a, vec3 b);

vec4 vec4_mul_float(vec4 v, float f);
vec4 vec4_add(vec4 a, vec4 b);

mat4 mat4_mul_float(mat4 m, float f);
vec4 mat4_mul_vec4(mat4 m, vec4 v);
mat4 mat4_mul_mat4(mat4 a, mat4 b);
float determinant(mat4 m);
mat4 transposed(mat4 m);
mat4 adjugate(mat4 m);
mat4 inverse(mat4 m);

quat quat_angle_axis(float angle, vec3 axis);
vec3 quat_rotate(quat q, vec3 v);

/**************************************/
vec2 build_vec2(float x, float y);
vec3 build_vec3(float x, float y, float z);
vec4 build_vec4(float x, float y, float z, float w);
quat build_quat(float x, float y, float z, float w);
vec4 to_vec4(vec3 v, float w);
vec3 to_vec3(vec4 v);

#ifdef __cplusplus
}
#endif

#endif
 