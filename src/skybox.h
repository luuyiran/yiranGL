#ifndef __SKY_BOX_H__
#define __SKY_BOX_H__

typedef struct skybox_t {
    unsigned int          program;
    unsigned int          cubemap;
    struct model_t       *model;
} skybox_t;

skybox_t *skybox_create(unsigned int cube_map);
void skybox_delete(skybox_t *s);
void skybox_render(render_config *cfg, skybox_t *s);

#endif 
