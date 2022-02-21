#ifndef __DRAW_H__
#define __DRAW_H__
#include "model.h"

void init_buffer(model_t *model);
void load_texture(model_t *model, char *path, char *name, texture_type type);
void model_add_texture(model_t *mdl, char *pic, texture_type type, int texture_should_flip);
void model_add_texture_with(model_t *mdl, GLuint id, texture_type type, const char* name);
int draw_obj_model(model_t *model, GLuint shader);

void update_mesh_mtl_info(model_t *model);
void init_uniforms(model_t *model);

#endif
