#ifndef __UTILS_H__
#define __UTILS_H__

#include "mat4.h"
#include "model.h"

#define         YIRAN_TRUE      1
#define         YIRAN_FALSE     0

char *str_alloc(const char *str);
char *dir_name(const char* path);
void correct_path(char *dir, char *name);
char *path_to_file(char *path, char *name);
void to_lower(char *str);
int is_space(int c);
int is_num(int c);

char *read_file(const char *name);
/* get word from buffpos, separated by space */
char *get_next_word(char *word, char *buffpos);
char *read_line(char **pos, char *line, int len);
char *after_slash(char *pos);

vec2 read_vec2(char *line);
vec3 read_vec3(char *line);

int is_same(vec3 *pos, vec2 *uvs, int i, int j);
int is_unique_vertices(model_t *model);

/* print help function */
void print_texture(texture_t *texture);
void print_material(material_t *material);
void print_mesh(mesh_t *mesh);
void print_model(model_t *model);

#endif
