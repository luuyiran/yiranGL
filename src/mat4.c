#include <math.h>
#include <stdio.h>
#include <string.h>
#include "mat4.h"

mat4 lookAt(vec3 eye, vec3 target, vec3 up) {
    mat4 ret;
    vec3 x_axis, y_axis, z_axis;
    z_axis = vec3_normalize(vec3_sub(eye, target));
    y_axis = vec3_normalize(up);
    x_axis = vec3_normalize(vec3_cross(y_axis, z_axis));
    
    ret.col[0] = build_vec4(x_axis.x,   y_axis.x, z_axis.x, 0.f);
    ret.col[1] = build_vec4(x_axis.y,   y_axis.y, z_axis.y, 0.f);
    ret.col[2] = build_vec4(x_axis.z,   y_axis.z, z_axis.z, 0.f);
    ret.col[3] = build_vec4(-vec3_dot(x_axis, eye), -vec3_dot(y_axis, eye), -vec3_dot(z_axis, eye), 1.f);

    return ret;
}


mat4 ortho(float fov, float aspect, float n, float f) {
    mat4 ret;
    float t = n * tan(RADIANS(0.5f * fov));
    float b = -t;
    float r = t * aspect;
    float l = -r;

    ret.col[0] = build_vec4(2.f / (r - l), 0.f, 0.f, 0.f);
    ret.col[1] = build_vec4(0.f, 2.f / (t - b), 0.f, 0.f);
    ret.col[2] = build_vec4(0.f, 0.f, -2.f / (f - n), 0.f);
    ret.col[3] = build_vec4(-((r + l) / (r - l)), -((t + b) / (t - b)), -((f + n) / (f-n)), 1.f);

    return ret;
}

mat4 perspective(float fov, float aspect, float n, float f) {
    mat4 ret;
    float t = n * tan(RADIANS(0.5f * fov));
    float b = -t;
    float r = t * aspect;
    float l = -r;
    ret.col[0] = build_vec4(2*n/(r-l), 0.f, 0.f,  0.f);
    ret.col[1] = build_vec4(0.f,2*n/(t-b), 0.f,  0.f);
    ret.col[2] = build_vec4((r+l)/(r-l), (t+b)/(t-b),-(f+n)/(f-n), -1.f);
    ret.col[3] = build_vec4(0.f, 0.f,-2*n*f/(f-n),  0.f);
    return ret;
}


mat4 perspective_fast(float fovy, float aspect, float near, float far) {
    mat4 ret;
    float q, A, B, C;
    q = 1.0f / tan(RADIANS(0.5f * fovy));
    A = q / aspect;
    B = (near + far) / (near - far);
    C = (2.0f * near * far) / (near - far);

    ret.col[0] = build_vec4(  A, 0.f, 0.f,  0.f);
    ret.col[1] = build_vec4(0.f,   q, 0.f,  0.f);
    ret.col[2] = build_vec4(0.f, 0.f,   B, -1.f);
    ret.col[3] = build_vec4(0.f, 0.f,   C,  0.f);

    return ret;
}


mat4 view_port_matrix(int width, int height) {
    mat4 ret;
    float w2 = width / 2.f;
    float h2 = height / 2.f;

    ret.col[0] = build_vec4(w2, 0.f,0.f,0.f);
    ret.col[1] = build_vec4(0.f,h2,0.f,0.f);
    ret.col[2] = build_vec4(0.f,0.f,1.f,0.f);
    ret.col[3] = build_vec4(w2-0.5, h2-0.5, 0.f, 1.f);
    return ret;
}


mat4 mat4_identity(void) {
    mat4 ret = {{
        {1, 0, 0, 0},       /* first colum */
        {0, 1, 0, 0},       /* second colum */
        {0, 0, 1, 0},       /* third colum */
        {0, 0, 0, 1},       /* fourth colum */
    }};
    return ret;
}

mat4 scale(float x) {
    mat4 ret;
    ret.col[0] = build_vec4(x,   0.f, 0.f, 0.f);
    ret.col[1] = build_vec4(0.f,   x, 0.f, 0.f);
    ret.col[2] = build_vec4(0.f, 0.f,   x, 0.f);
    ret.col[3] = build_vec4(0.f, 0.f, 0.f, 1.f);
    return ret;
}

mat4 translate(vec3 v) {
    mat4 ret;
    ret.col[0] = build_vec4(1.f, 0.f, 0.f, 0.f);
    ret.col[1] = build_vec4(0.f, 1.f, 0.f, 0.f);
    ret.col[2] = build_vec4(0.f, 0.f, 1.f, 0.f);
    ret.col[3] = build_vec4(v.x, v.y, v.z, 1.f);

    return ret;
}

static mat4 __rotate(float angle, float x, float y, float z) {
    mat4 ret;
    float x2, y2, z2, rads, c, s, omc;
    x2 = x * x;
    y2 = y * y;
    z2 = z * z;
    rads = RADIANS(angle);
    c = cos(rads);
    s = sin(rads);
    omc = 1.0f - c;

    ret.col[0] = build_vec4(x2 * omc + c, y * x * omc + z * s, x * z * omc - y * s, 0.f);
    ret.col[1] = build_vec4(x * y * omc - z * s, y2 * omc + c, y * z * omc + x * s, 0.f);
    ret.col[2] = build_vec4(x * z * omc + y * s, y * z * omc - x * s, z2 * omc + c, 0.f);
    ret.col[3] = build_vec4(0.f, 0.f, 0.f, 1.f);

    return ret;
}

mat4 rotate(float angle, vec3 axis) {
    return __rotate(angle, axis.x, axis.y, axis.z);
}

mat4 rotate_x(float angle) {
        mat4 ret;
    float rads = RADIANS(angle);
    float c = cos(rads);
    float s = sin(rads);
    ret.col[0] = build_vec4(1, 0, 0, 0);
    ret.col[1] = build_vec4(0, c, s, 0);
    ret.col[2] = build_vec4(0,-s, c, 0);
    ret.col[3] = build_vec4(0, 0, 0, 1);
    return ret;
}

mat4 rotate_y(float angle) {
    mat4 ret;
    float rads = RADIANS(angle);
    float c = cos(rads);
    float s = sin(rads);
    ret.col[0] = build_vec4(c, 0,-s, 0);
    ret.col[1] = build_vec4(0, 1, 0, 0);
    ret.col[2] = build_vec4(s, 0, c, 0);
    ret.col[3] = build_vec4(0, 0, 0, 1);

    return ret;
}

mat4 rotate_z(float angle){
    mat4 ret;
    float rads = RADIANS(angle);
    float c = cos(rads);
    float s = sin(rads);
    ret.col[0] = build_vec4( c, s, 0, 0);
    ret.col[1] = build_vec4(-s, c, 0, 0);
    ret.col[2] = build_vec4( 0, 0, 1, 0);
    ret.col[3] = build_vec4( 0, 0, 0, 1);

    return ret;
}


vec3 vec3_normalize(vec3 v) {
    float inv_len = 0.f;
    float len_sq = v.x*v.x + v.y*v.y + v.z*v.z;
    if (len_sq < MAT4_EPSILON) {
        fprintf(stderr, " warning: vec3_normalize: len_sq: %.12f\n", len_sq);
        return v;
    }
    inv_len = 1.f / sqrt(len_sq);

    v.x *= inv_len;
    v.y *= inv_len;
    v.z *= inv_len;
    return v;
}

vec3 vec3_cross(vec3 a, vec3 b) {
    vec3 ret;
    ret.x = a.y * b.z - b.y * a.z;
    ret.y = a.z * b.x - b.z * a.x;
    ret.z = a.x * b.y - b.x * a.y;

    return ret;
}

float vec3_len(vec3 v) {
    float len_sq = v.x*v.x + v.y*v.y + v.z*v.z;
    if (len_sq < MAT4_EPSILON)
        return 0.f;
    return sqrt(len_sq);
}
 
float vec3_dot(vec3 a, vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3 vec3_mul_float(vec3 v, float f) {
    v.x *= f;
    v.y *= f;
    v.z *= f;
    return v;
}

vec3 vec3_add(vec3 a, vec3 b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return a;
}

vec3 vec3_sub(vec3 a, vec3 b) {
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    return a;
}

vec4 vec4_mul_float(vec4 v, float f) {
    v.x *= f;
    v.y *= f;
    v.z *= f;
    v.w *= f;
    return v;
}

vec4 vec4_add(vec4 a, vec4 b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    a.w += b.w;
    return a;
}

vec4 mat4_mul_vec4(mat4 m, vec4 v) {
    vec4 ret = build_vec4(
        m.col[0].x * v.x + m.col[1].x * v.y + m.col[2].x * v.z  + m.col[3].x * v.w,
        m.col[0].y * v.x + m.col[1].y * v.y + m.col[2].y * v.z  + m.col[3].y * v.w,
        m.col[0].z * v.x + m.col[1].z * v.y + m.col[2].z * v.z  + m.col[3].z * v.w,
        m.col[0].w * v.x + m.col[1].w * v.y + m.col[2].w * v.z  + m.col[3].w * v.w
    );
    return ret;
}

mat4 mat4_mul_float(mat4 m, float f) {
    int i = 0;
    float *v = (float *)&m;
    for (i = 0; i < 16; i++) 
        v[i] *= f;
    return m;
}

mat4 transposed(mat4 m) {
    mat4 ret;
    ret.col[0] = build_vec4(m.col[0].x, m.col[1].x, m.col[2].x, m.col[3].x);
    ret.col[1] = build_vec4(m.col[0].y, m.col[1].y, m.col[2].y, m.col[3].y);
    ret.col[2] = build_vec4(m.col[0].z, m.col[1].z, m.col[2].z, m.col[3].z);
    ret.col[3] = build_vec4(m.col[0].w, m.col[1].w, m.col[2].w, m.col[3].w);
    return ret;
}

#define M4_3X3MINOR(c0, c1, c2, r0, r1, r2) \
    (v[c0 * 4 + r0] * (v[c1 * 4 + r1] * v[c2 * 4 + r2] - v[c1 * 4 + r2] * v[c2 * 4 + r1]) - \
     v[c1 * 4 + r0] * (v[c0 * 4 + r1] * v[c2 * 4 + r2] - v[c0 * 4 + r2] * v[c2 * 4 + r1]) + \
     v[c2 * 4 + r0] * (v[c0 * 4 + r1] * v[c1 * 4 + r2] - v[c0 * 4 + r2] * v[c1 * 4 + r1]))

float determinant(mat4 m) {
    float *v = (float *)&m;
    return  v[0] * M4_3X3MINOR(1, 2, 3, 1, 2, 3)
          - v[4] * M4_3X3MINOR(0, 2, 3, 1, 2, 3)
          + v[8] * M4_3X3MINOR(0, 1, 3, 1, 2, 3)
          - v[12]* M4_3X3MINOR(0, 1, 2, 1, 2, 3);
}

mat4 adjugate(mat4 m) {
    mat4 cofactor;
    float *v = (float *)&m;
    float *cv = (float *)&cofactor;
    
    cv[0] = M4_3X3MINOR(1, 2, 3, 1, 2, 3);
    cv[1] = -M4_3X3MINOR(1, 2, 3, 0, 2, 3);
    cv[2] = M4_3X3MINOR(1, 2, 3, 0, 1, 3);
    cv[3] = -M4_3X3MINOR(1, 2, 3, 0, 1, 2);

    cv[4] = -M4_3X3MINOR(0, 2, 3, 1, 2, 3);
    cv[5] = M4_3X3MINOR(0, 2, 3, 0, 2, 3);
    cv[6] = -M4_3X3MINOR(0, 2, 3, 0, 1, 3);
    cv[7] = M4_3X3MINOR(0, 2, 3, 0, 1, 2);

    cv[8] = M4_3X3MINOR(0, 1, 3, 1, 2, 3);
    cv[9] = -M4_3X3MINOR(0, 1, 3, 0, 2, 3);
    cv[10] = M4_3X3MINOR(0, 1, 3, 0, 1, 3);
    cv[11] = -M4_3X3MINOR(0, 1, 3, 0, 1, 2);

    cv[12] = -M4_3X3MINOR(0, 1, 2, 1, 2, 3);
    cv[13] = M4_3X3MINOR(0, 1, 2, 0, 2, 3);
    cv[14] = -M4_3X3MINOR(0, 1, 2, 0, 1, 3);
    cv[15] = M4_3X3MINOR(0, 1, 2, 0, 1, 2);

    return transposed(cofactor);
}

mat4 inverse(mat4 m) {
    mat4 adj;
    float det = determinant(m);
    if (fabs(det) < MAT4_EPSILON) {
        fprintf(stderr, "WARNING: Trying to invert a matrix with a zero determinant!\n");
        return mat4_identity();
    }
    adj = adjugate(m);
    return mat4_mul_float(adj, 1.f / det);
}


mat4 mat4_mul_mat4(mat4 a, mat4 b) {
    mat4 ret;
    ret.col[0] = mat4_mul_vec4(a, b.col[0]);
    ret.col[1] = mat4_mul_vec4(a, b.col[1]);
    ret.col[2] = mat4_mul_vec4(a, b.col[2]);
    ret.col[3] = mat4_mul_vec4(a, b.col[3]);
    return ret;
}

quat quat_angle_axis(float angle, vec3 axis) {
    float s = 0.f;
    float c = 0.f;
    float rads = RADIANS(angle);
    axis = vec3_normalize(axis);
    s = sin(rads * 0.5);
    c = cos(rads * 0.5);
    return build_quat(s * axis.x,
                      s * axis.y,
                      s * axis.z,
                      c);
}

vec3 quat_rotate(quat quat, vec3 v) {
    vec3 quat_vec = build_vec3(quat.x, quat.y, quat.z);
    vec3 a = vec3_mul_float(quat_vec, 2.f * vec3_dot(quat_vec, v));
    vec3 b = vec3_mul_float(v, quat.w * quat.w - vec3_dot(quat_vec, quat_vec));
    vec3 c = vec3_mul_float(vec3_cross(quat_vec, v), 2.f * quat.w);
    return vec3_add(a, vec3_add(b, c));
}

vec2 build_vec2(float x, float y) {
    vec2 ret;
    ret.x = x;
    ret.y = y;
    return ret;
}

vec3 build_vec3(float x, float y, float z) {
    vec3 ret;
    ret.x = x;
    ret.y = y;
    ret.z = z;
    return ret;
}

vec4 build_vec4(float x, float y, float z, float w) {
    vec4 ret;
    ret.x = x;
    ret.y = y;
    ret.z = z;
    ret.w = w;
    return ret;
}

quat build_quat(float x, float y, float z, float w) {
    quat ret;
    ret.x = x;
    ret.y = y;
    ret.z = z;
    ret.w = w;
    return ret;
}

vec4 to_vec4(vec3 v, float w) {
    return build_vec4(v.x, v.y, v.z, w);
}

vec3 to_vec3(vec4 v) {
    return build_vec3(v.x, v.y, v.z);
}
