#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "shader.h"

#define ERR_BUF_LEN  1024

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

static char *get_dir(const char *glsl_file) {
    char* dir = NULL;
    int i = 0;
    int len = 0;

    len = strlen(glsl_file);
    dir = (char *)malloc(len + 1);
    assert(dir);
    memcpy(dir, glsl_file, len);
    i = len - 1;
    while (i >= 0 && dir[i] != '/' && dir[i] != '\\') {
        i--;
    }
    if (i == 0) dir[0] = '\0';  /* root dir */
    else dir[i + 1] = '\0';

    return dir;
}

static char *get_inc_file(const char *line) {
    int len = 0;
    char *name = NULL;
    const char *p1 = NULL;
    const char *p2 = NULL;
    if (NULL == line || '\0' == line[0])
        return NULL;
    p1 = strstr(line, "<");
    p2 = strstr(line, ">");
    p1++;
    if (NULL == p1 || NULL == p2)
        return NULL;
    len = p2 - p1;
    name = (char *)malloc(len + 1);
    if (NULL == name)
        return NULL;
    memcpy(name, p1, p2 - p1);
    name[len] = '\0';
    return name;
}

static void print_shader(const char *text) {
    int line = 1;
    if (NULL == text || '\0' == text[0])
        return;
    printf("\n(%3i) ", line);
    while (text && *text++) {
        if (*text == '\n') printf("\n(%3i) ", ++line);
        else if (*text == '\r') {}
        else printf("%c", *text);
    }
    printf("\n");
    printf("---------------------------\n");
}

static char *read_glsl_from(const char *glsl_file) {
    size_t len = 0;
    char line[256] = {0};
    char inc_file[256] = {0};
    char *inc_name = NULL;
    char *inc = NULL;
    int inc_len = 0;
    char *dir = NULL;
    char *buf = NULL;
    FILE *file = NULL;
    const char *include = "#include";

    file = fopen(glsl_file, "rb");
    if(NULL == file) {
        fprintf(stderr, "fopen \"%s\" faied!\n", glsl_file);
        exit(-1);
    }

    fseek(file, 0, SEEK_END);
    len = ftell(file);
    rewind(file);

    buf = (char *)calloc(1, len + 1);
    if(NULL == buf) {
        fprintf(stderr, "calloc for \"%s\" error!\n", glsl_file);
        exit(-1);
    }

    while (fgets(line, 256, file)) {
        /* handle #include directives inside the shader source code. 
         * not robust enough.
         */
        if (memcmp(line, include, MIN(strlen(include), strlen(line))) == 0) {
            inc_name = get_inc_file(line);
            assert(inc_name);
            dir = get_dir(glsl_file);
            sprintf(inc_file, "%s%s", dir, inc_name);
            inc = read_glsl_from(inc_file);
            inc_len = strlen(inc);
            buf = (char *)realloc(buf, len + inc_len + 1);
            memcpy(buf + strlen(buf), inc, inc_len + 1);
            free(inc_name);
            free(dir);
            free(inc);
        } else {
            memcpy(buf + strlen(buf), line, strlen(line) + 1);
        }
    }

    fclose(file);
    return buf;
}

static void compile_shader(GLuint prog, const char *filename, GLenum type) {
    GLuint shader;
    const char* buf = NULL;
    GLint status = GL_FALSE;
    GLchar error_log[ERR_BUF_LEN] = {0};

    assert(prog);
    if (!prog || !filename)        
        return;

    buf = read_glsl_from(filename);
    assert(buf);
    shader = glCreateShader(type);
    assert(shader);
    glShaderSource(shader, 1, &buf, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        glGetShaderInfoLog(shader, ERR_BUF_LEN, NULL, error_log);
        print_shader(buf);
        fprintf(stderr, "\n%s\n", error_log);
        fprintf(stderr, "glCompileShader failed at: \"%s\"\n", filename);
        exit(-1);
    }

    glAttachShader(prog, shader);
    glDeleteShader(shader);
    free((void *)buf);
}

GLuint shader_create(const char* vert, const char* frag, const char* geom) {
    GLuint prog = 0;
    GLint status = GL_FALSE;
    GLchar error_log[ERR_BUF_LEN] = {0};

    prog = glCreateProgram();
    assert(prog);

    compile_shader(prog, vert, GL_VERTEX_SHADER);
    compile_shader(prog, frag, GL_FRAGMENT_SHADER);
    compile_shader(prog, geom, GL_GEOMETRY_SHADER);

    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        glGetProgramInfoLog(prog, ERR_BUF_LEN, NULL, error_log);
        fprintf(stderr, "Program link failed:\n%s\n", error_log);
        exit(-1);
    }
    return prog;
}

void shader_use(GLuint shader) { 
    glUseProgram(shader); 
}

void shader_delete(GLuint shader) {
    glDeleteProgram(shader);
}

#if 0
void shader_set_int(GLuint shader, GLchar *name, GLint value) {
    glUniform1i(glGetUniformLocation(shader, name), value);
}

void shader_set_float(GLuint shader, GLchar *name, GLfloat value) {
    glUniform1f(glGetUniformLocation(shader, name), value);
}

void shader_set_2float(GLuint shader, GLchar *name, GLfloat x, GLfloat y) {
    glUniform2f(glGetUniformLocation(shader, name), x, y);
}

void shader_set_3float(GLuint shader, GLchar *name, GLfloat x, GLfloat y, GLfloat z) {
    glUniform3f(glGetUniformLocation(shader, name), x, y, z);
}

void shader_set_4float(GLuint shader, GLchar *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    glUniform4f(glGetUniformLocation(shader, name), x, y, z, w);
}

void shader_set_vec2(GLuint shader, GLchar *name, const GLfloat *value) {
    glUniform2fv(glGetUniformLocation(shader, name), 1, value);
}

void shader_set_vec3(GLuint shader, GLchar *name, const GLfloat *value) {
    glUniform3fv(glGetUniformLocation(shader, name), 1, value);
}

void shader_set_vec4(GLuint shader, GLchar *name, const GLfloat *value) {
    glUniform4fv(glGetUniformLocation(shader, name), 1, value);
}

void shader_set_mat2(GLuint shader, GLchar *name, const GLfloat *mat2) {
    glUniformMatrix2fv(glGetUniformLocation(shader, name), 1, GL_FALSE, mat2);
}

void shader_set_mat3(GLuint shader, GLchar *name, const GLfloat *mat3) {
    glUniformMatrix3fv(glGetUniformLocation(shader, name), 1, GL_FALSE, mat3);
}

void shader_set_mat4(GLuint shader, GLchar *name, const GLfloat *mat4) {
    glUniformMatrix4fv(glGetUniformLocation(shader, name), 1, GL_FALSE, mat4);
}

#endif

