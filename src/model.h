#ifndef __OBJ_MODEL_H__
#define __OBJ_MODEL_H__
#include "glad.h"
#include "mat4.h"
#include "uniforms.h"

typedef enum texture_type {
    DIFFUSE_TEXTURE = 0,
    SPECULAR_TEXTURE,
    AMBIENT_TEXTURE,
    NORMAL_TEXTURE,
    BUMP_TEXTURE,
    DISPLACEMENT_TEXTURE,
    EMISSIVE_TEXTURE,
    ROUGHNESS_TEXTURE,
    CUBEMAP_TEXTURE,
    AO_TEXTURE,
    HEIGHT_TEXTURE,
    TEXTURE_COUNT
} texture_type;

typedef struct texture_t {
    char        *name;
    unsigned int id;
    texture_type type;
} texture_t;

typedef struct material_t {
    char       *name;
    vec3        diffuse;       /* Kd, diffuse component */
    vec3        ambient;       /* Ka, ambient component */
    vec3        specular;      /* Ks, specular component */
    float       shininess;     /* Ns, specular exponent */
    texture_t **textures;      /* pointer array, pointer to model_t's texture */
} material_t;

typedef struct mesh_t {
    GLuint           vao;
    GLuint           ebo;
    char            *name;
    unsigned int    *indices;
    material_t      *material;
    uniform_mtl     *uniform_mtl;
} mesh_t;

typedef struct model_t {
    char        *name;
    char        *dir;
    char        *mtl;
    vec3        *position;                
    vec3        *normal;
    vec3        *tangent;                 
    vec2        *tex_coords;               
    mesh_t      *meshes;
    texture_t   *textures;      /* all the png, jpg, bmp.. textures in this array */
    material_t  *material;      /* read *.mtl into this array */

    float        scale;
    vec3         translation;   /* x, y, z axis translations */
    vec3         rotation;      /* angles of x, y, z axis rotations */
    mat4         model_matrix;
    vec3         bbox[2];       /* box[0]:min  box[1]:max */

    GLuint       position_buffer;
    GLuint       normal_buffer;
    GLuint       uv_buffer;
    GLuint       tangent_buffer;

    uniform_gui_ctl *uniform_gui;
} model_t;

model_t *load_obj_model(const char *obj_model, int texture_should_flip);
model_t *load_cube_model(void);
model_t *load_sphere_model(float r, int edges);
model_t *load_torus_model(float R, float r, int nsides, int nrings);
model_t *load_teapot_model(int divs);
model_t *load_vase_model(int edges);
void model_delete(model_t *model);

void model_scale(model_t *model, float scale);
void model_translate(model_t *model, vec3 translation);
void model_rotate_x(model_t *model, float angle);
void model_rotate_y(model_t *model, float angle);
void model_rotate_z(model_t *model, float angle);
void model_rotate(model_t *model, vec3 rotation);
mat4 update_model_matrix(model_t *model);


#endif
