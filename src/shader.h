#ifndef __SHADER_H__
#define __SHADER_H__

#include "glad.h"

GLuint shader_create(const char* vert, const char* frag, const char* geom);
void shader_use(GLuint id);
void shader_delete(GLuint id);

#if 0
void shader_set_int(GLuint shader, GLchar *name, GLint value);

void shader_set_float (GLuint shader, GLchar *name, GLfloat x);
void shader_set_2float(GLuint shader, GLchar *name, GLfloat x, GLfloat y);
void shader_set_3float(GLuint shader, GLchar *name, GLfloat x, GLfloat y, GLfloat z);
void shader_set_4float(GLuint shader, GLchar *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w);

void shader_set_vec2(GLuint shader, GLchar *name, const GLfloat *value);
void shader_set_vec3(GLuint shader, GLchar *name, const GLfloat *value);
void shader_set_vec4(GLuint shader, GLchar *name, const GLfloat *value);

void shader_set_mat2(GLuint shader, GLchar *name, const GLfloat *mat2);
void shader_set_mat3(GLuint shader, GLchar *name, const GLfloat *mat3);
void shader_set_mat4(GLuint shader, GLchar *name, const GLfloat *mat4);
#endif 

#endif
