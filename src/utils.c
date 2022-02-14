#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "array.h"
#include "utils.h"

char *str_alloc(const char *str) {
    char *copy = NULL;
    unsigned int len = 0;

    if (NULL == str || '\0' == str[0])
        return NULL;

    len = strlen(str);
    copy = (char *)malloc(len + 1);
    if (copy == NULL)
        return NULL;
    memcpy(copy, str, len);
    copy[len] = '\0';
    return copy;
}

char *dir_name(const char* path) {
    char* dir = NULL;
    int i = 0;

    assert(path);
    dir = str_alloc(path);
    assert(dir);
    i = strlen(dir) - 1;
    while (i >= 0 && dir[i] != '/' && dir[i] != '\\') {
        i--;
    }
    if (i == 0) dir[0] = '\0';  /* root dir */
    else dir[i + 1] = '\0';

    return dir;
}

void correct_path(char *dir, char *name) {
    int i = 0;
    int len = 0;
    if (!dir || !name)
        return;
    len = strlen(name);
    if (strchr(dir, '\\')) {
        /* win32 style path: C:\windows\fonds\ */
        for (i = 0; i < len; i++)
            if (name[i] == '/')
                name[i] = '\\';
    } else {
        /* unix style path: /usr/include/GL/ */
        for (i = 0; i < len; i++)
            if (name[i] == '\\')
                name[i] = '/';
    }
}

char *path_to_file(char *path, char *name) {
    int len = 0;
    char *full_name = NULL;

    if (NULL == name || name[0] == '\0')
        return NULL;
    if (NULL == path || path[0] == '\0')
        return str_alloc(name);

    correct_path(path, name);

    len = strlen(path) + strlen(name) + 1;
    full_name = (char *)malloc(len);
    assert(full_name);
    memset(full_name, 0, len);
    sprintf(full_name, "%s%s", path, name);
    return full_name;
}

void to_lower(char *str) {
    if (NULL == str)    return;
    while (*str) {
        if (*str >= 'A' && *str <= 'Z')
            *str += 32;
        str++;
    }
}

int is_space(int c) {
    return c == ' '  || c == '\t' || c == '\n' || \
           c == '\r' || c == '\f' || c == '\v';
}

int is_num(int c) { 
    return (c >= '0' && c <= '9') || c == '-' || c == '+'; 
}

/* get word from buffpos, separated by space */
char *get_next_word(char *word, char *buffpos) {
    if (NULL == word || NULL == buffpos) return NULL;

    if ('\0' == *buffpos) return NULL;

    while (is_space(*buffpos)) buffpos++;

    while (*buffpos) {
        if (is_space(*buffpos)) break;
        *word++ = *buffpos++;
    }
    while (is_space(*buffpos)) buffpos++;

    *word = '\0';
    return buffpos;
}

vec2 read_vec2(char *line) {
    int i = 0;
    float *v = NULL;
    vec2 vec = {0};
    char word[128] = {0};
    while (!is_num(*line)) line++;
    v = &vec.x;
    for (i = 0; i < 2; i++) {
        line = get_next_word(word, line);
        v[i] = atof(word);
    }
    return vec;
}

vec3 read_vec3(char *line) {
    int i = 0;
    float *v = NULL;
    vec3 vec = {0};
    char word[128] = {0};
    /* omit 'v', 'vn', ' ',  or '\t' */
    while (!is_num(*line)) line++;
    
    v = &vec.x;
    for (i = 0; i < 3; i++) {
        line = get_next_word(word, line);
        v[i] = atof(word);
    }

    return vec;
}

char *read_line(char **pos, char *line, int len) {
    char *read = *pos;
    if (pos == NULL || line == NULL|| len < 2 || *pos == 0 ) 
        return NULL;

    /* omit blank line 、 empty of the front line or last line's \r \n */
    while (*read != 0 && is_space(*read)) {
        read++;
    }

    /* if reached the EOF */
    if (*read == '\0')
        return NULL;

    while (*read != 0) {
        /* reached the end of current line */
        if ('\n' == *read || '\r' == *read) 
            break;
        *line++ = *read++;
        /* avoid buf over flow */
        if (read - *pos >= len) 
            break;
    }

    *line = '\0';
    *pos = read;    /* next line */
    return line;
}

char *after_slash(char *pos) {
    assert(pos);
    if (*pos == '\0') return pos;
    while (*pos != '/') {
        pos++;
        if (*pos == '\0')
            return pos;
    }
    return pos + 1;
}

char *read_file(const char *name) {
    char *buf = NULL;
    FILE *file = NULL;
    long file_size = 0;

    file = fopen(name, "rb");
    if (NULL == file) {
        fprintf(stderr, "fopen failed: can't open file \"%s\".\n", name);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    if (-1L == file_size) {
        fprintf(stderr, "obtain file size failed: \"%s\".\n", name);
        fclose(file);
        return NULL;
    }
    rewind(file);
    buf = (char *)malloc(file_size + 1);
    if (buf == NULL) {
        fprintf(stderr, "malloc failed: for \"%s\".\n", name);
        fclose(file);
        return NULL;
    }
    memset(buf, 0, file_size + 1);
    if (fread(buf, 1, file_size, file) != (size_t)file_size) {
        fprintf(stderr, "fread failed: \"%s\".\n", name);
        fclose(file);
        free(buf);
        return NULL;
    }

    return buf;
} 


/* same position may have different uv coordinate */
int is_same(vec3 *pos, vec2 *uvs, int i, int j) {
    int p = YIRAN_TRUE;
    int u = YIRAN_TRUE;

    if (pos)
        p = (fabs(pos[i].x - pos[j].x) < MAT4_EPSILON) && \
            (fabs(pos[i].y - pos[j].y) < MAT4_EPSILON) && \
            (fabs(pos[i].z - pos[j].z) < MAT4_EPSILON);

    if (uvs) 
        u = (fabs(uvs[i].x - uvs[j].x) < MAT4_EPSILON) && \
            (fabs(uvs[i].y - uvs[j].y) < MAT4_EPSILON);

    return p && u;
}

int is_unique_vertices(model_t *model) {
    int i = 0;
    int j = 0;
    int size = 0;
    vec3 *pos = NULL;
    vec2 *uv = NULL;
    if (NULL == model || NULL == model->position)  return YIRAN_TRUE;
    pos = model->position;
    uv = model->tex_coords;
    size = array_size(pos);
    for (i = 0; i < size - 1; i++) {
        for (j = i + 1; j < size; j++) {
            if (is_same(pos, uv, i, j)) {
                return YIRAN_FALSE;
            }
        }
    }
    return YIRAN_TRUE;
}

void print_texture(texture_t *texture) {
    if (NULL == texture) return;
    printf("%s\t:", texture->name);
    switch (texture->type) {
        case DIFFUSE_TEXTURE:        printf("diffuse\n");       break;
        case SPECULAR_TEXTURE:       printf("specular\n");      break;
        case AMBIENT_TEXTURE:        printf("ambient\n");       break;
        case NORMAL_TEXTURE:         printf("normal\n");        break;
        case BUMP_TEXTURE:           printf("bump\n");          break;
        case DISPLACEMENT_TEXTURE:   printf("displacement\n");  break;
        case EMISSIVE_TEXTURE:       printf("emissive\n");      break;
        case ROUGHNESS_TEXTURE:      printf("roughness\n");     break;
        case AO_TEXTURE:             printf("ssao\n");          break;
        default:                     printf("unknown\n");       break;
    }
}

void print_material(material_t *material) {
    int i = 0;
    int n = 0;
    if (NULL == material)   return;

    printf("material : %s\n", material->name);
    printf("ambient  : (%.3f, %.3f, %.3f)\n", material->ambient.x, material->ambient.y, material->ambient.z);
    printf("diffuse  : (%.3f, %.3f, %.3f)\n", material->diffuse.x, material->diffuse.y, material->diffuse.z);
    printf("specular : (%.3f, %.3f, %.3f)\n", material->specular.x, material->specular.y, material->specular.z);
    printf("shiness  :  %.3f\n", material->shininess);

    n = array_size(material->textures);
    for (i = 0; i < n; i++)
        print_texture(material->textures[i]);
    printf("texture count   : %d\n", n);
    printf("------------------------------------------\n");
}

void print_mesh(mesh_t *mesh) {
    if (NULL == mesh)   return;

    printf("mesh     : %s\n", mesh->name);
    printf("usemtl   : %s\n", mesh->material ? mesh->material->name : "NULL");
    printf("triangles: %ld\n", array_size(mesh->indices) / 3);

    if (mesh->material)
        print_material(mesh->material);
    else
        printf("------------------------------------------\n");
}

void print_model(model_t *model) {
    if (NULL == model) return;
    printf("--------------- model info ---------------\n");
    printf("file name           : %s\n", model->name);
    printf("directory           : %s\n", model->dir);
    printf("material            : %s\n", model->mtl);
    printf("vertices            : %ld\n", array_size(model->position));
    printf("normals             : %ld\n", array_size(model->normal));
    printf("tex_coords          : %ld\n", array_size(model->tex_coords));
    printf("materials           : %ld\n", array_size(model->material));
    printf("textures            : %ld\n", array_size(model->textures));
    printf("meshs               : %ld\n", array_size(model->meshes));
    printf("vertex normals      : %s\n", model->normal ? "yes" : "no");
    printf("texture coordinates : %s\n", model->tex_coords ? "yes" : "no");
    printf("bounding box        : (%.3f, %.3f, %.3f) - (%.3f, %.3f, %.3f)\n", \
        model->bbox[0].x, model->bbox[0].y, model->bbox[0].z, \
        model->bbox[1].x, model->bbox[1].y, model->bbox[1].z);
    printf("------------------------------------------\n");
#if 0
{
    texture_t *t = NULL;
    material_t *m = NULL;
    mesh_t *mesh = NULL;

    for (t = array_begin(model->textures); t != array_end(model->textures); t++)
        print_texture(t);
    for (m = array_begin(model->material); m != array_end(model->material); m++)
        print_material(m);
    for (mesh = array_begin(model->meshes); mesh != array_end(model->meshes); mesh++)
        print_mesh(mesh);
}
#endif
}
