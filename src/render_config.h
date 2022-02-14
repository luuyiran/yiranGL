#ifndef __SCENE_H__
#define __SCENE_H__
#include "mat4.h"

typedef struct render_config render_config;
typedef void (*SET_UNIFORMS)(render_config *cfg, mat4 modelMatrix);

typedef struct shader_config {
    unsigned int shader;
    SET_UNIFORMS set_matrix;
} shader_config;

struct render_config {
    int                      width;
    int                      height;
    struct model_t         **models;
    struct skybox_t         *skybox;
    struct camera_t         *camera;
    struct window_t         *window;
    struct nk_gui_t         *gui;
    struct shader_config    *shaders;

    int                      fps;
    int                      vsynch;           /* vertical synchronization */
    double                   delta_time;       /* s */
    double                   app_time_gpu;     /* ms */
    double                   gui_time_gpu;     /* ms */
    double                   event_time_cpu;   /* ms */
    double                   gui_time_cpu;     /* ms */
    double                   swap_time_wait;   /* ms */
};

render_config *config_init(int width, int height);
void config_delete(render_config *cfg);
void config_add_model(render_config *cfg, struct model_t *m);
void config_set_camera(render_config *cfg, struct camera_t *cmr);
void config_set_skybox(render_config *cfg, unsigned int cube);
void config_add_shader(render_config *cfg, unsigned int shader, SET_UNIFORMS set);
void config_set_window(render_config *cfg, struct window_t *window);
void config_window(render_config *cfg, const char *title);
void config_shader_matrix(render_config *cfg, mat4 modelMatrix);
void render_loop(render_config *cfg);


#endif
