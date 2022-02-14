#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "shader.h"
#include "model.h"
#include "draw.h"
#include "render_config.h"
#include "skybox.h"

extern void config_shader_matrix(render_config *cfg, mat4 model_matrix);

skybox_t *skybox_create(unsigned int cube_map) {
    skybox_t *skybox = NULL;
    assert(cube_map);
    if (!cube_map) return NULL;

    skybox = (skybox_t *)calloc(1, sizeof(skybox_t));
    assert(skybox);
    if (!skybox) return NULL;

    skybox->model = load_cube_model();
    assert(skybox->model);
    if (!skybox->model) {
        free(skybox);
        return NULL;
    }

    skybox->program = shader_create("shaders/skybox.vert", "shaders/skybox.frag", NULL);
    assert(skybox->program);
    if (!skybox->program) {
        model_delete(skybox->model);
        free(skybox);
        return NULL;
    }

    skybox->cubemap = cube_map;
    model_add_cubemap(skybox->model, cube_map);

    return skybox;
}


void skybox_delete(skybox_t *skybox) {
    if (skybox == NULL)  return;
    model_delete(skybox->model);
    glDeleteProgram(skybox->program);
    free(skybox);
}

void skybox_render(render_config *cfg, skybox_t *skybox) {
    assert(cfg);
    assert(skybox);
    assert(skybox->program);
    assert(skybox->model);
    if (!skybox || !skybox->program || !skybox->model)
        return;
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDepthFunc(GL_LEQUAL); 
    shader_use(skybox->program);
    config_shader_matrix(cfg, skybox->model->model_matrix);
    draw_obj_model(skybox->model, skybox->program);
    glDepthFunc(GL_LESS);     
}


