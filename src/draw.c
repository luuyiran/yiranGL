#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "glad.h"
#include "array.h"
#include "draw.h"
#include "uniforms.h"
#include "utils.h"
#include "shader.h"

#define CUBE_MAP_NAME  "_YIRAN_CUBE_MAP_"

/******************************* stb_image *******************************/
extern unsigned char *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
extern void stbi_image_free(void *retval_from_stbi_load);
extern void stbi_set_flip_vertically_on_load(int flag_true_if_should_flip);

void init_buffer(model_t *model) {
    GLuint ebo;
    GLuint vao;
    GLuint vbo[4];
    mesh_t *mesh = NULL;

    glCreateBuffers(4, vbo);
    model->position_buffer  = vbo[0];
    model->normal_buffer    = vbo[1];
    model->uv_buffer        = vbo[2];
    model->tangent_buffer   = vbo[3];

    assert(model->position);
    glNamedBufferStorage(model->position_buffer, array_size(model->position) * sizeof(vec3), (float *)model->position, 0);
    if (model->normal)
        glNamedBufferStorage(model->normal_buffer, array_size(model->normal) * sizeof(vec3), (float *)model->normal, 0);
    if (model->tex_coords)
        glNamedBufferStorage(model->uv_buffer, array_size(model->tex_coords) * sizeof(vec2), (float *)model->tex_coords, 0);
    if (model->tangent)
        glNamedBufferStorage(model->tangent_buffer, array_size(model->tangent) * sizeof(vec3), (float *)model->tangent, 0);


    for (mesh = array_begin(model->meshes); mesh != array_end(model->meshes); mesh++) {
        glCreateBuffers(1, &ebo);

        glCreateVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);

        glBindBuffer(GL_ARRAY_BUFFER, model->position_buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (GLubyte *)NULL);
        if (model->normal) {
            glBindBuffer(GL_ARRAY_BUFFER, model->normal_buffer);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,  sizeof(vec3), (GLubyte *)NULL);
        }
        if (model->tex_coords) {
            glBindBuffer(GL_ARRAY_BUFFER, model->uv_buffer);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (GLubyte *)NULL);
        }

        if (model->tangent) {
            glBindBuffer(GL_ARRAY_BUFFER, model->tangent_buffer);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (GLubyte *)NULL);
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glNamedBufferStorage(ebo, array_size(mesh->indices) * sizeof(unsigned int), mesh->indices, 0);

        mesh->vao = vao;
        mesh->ebo = ebo;
        glBindVertexArray(0);
    }
}

static texture_t *seach_texture(model_t *model, const char *name) {
    texture_t *itor = NULL;
    texture_t *texture = model->textures;

    if (NULL == texture)
        return NULL;

    for (itor = array_begin(texture); itor != array_end(texture); itor++) {
        if (strcmp(itor->name, name) == 0)
            return itor;
    }
    return NULL;
}

/* texture path: path/name */
void load_texture(model_t *model, char *path, char *name, texture_type type) {
    GLenum format = GL_RGB;
    GLenum internalformat = GL_RGB8;
    texture_t *p = NULL;
    texture_t texture;
    material_t *m = NULL;
    char *file_name = NULL;
    unsigned int textureID;
    unsigned char *data = NULL;
    int width, height, components;

    correct_path(model->dir, name);

    file_name = path_to_file(path, name);
    assert(file_name);

    p = seach_texture(model, file_name);
    m = array_back(model->material);
    if (p == NULL) {
        data = stbi_load(file_name, &width, &height, &components, 0);
        if (data) {
            if (components == 1) {       /* gray */
                format = GL_RED;
                internalformat = GL_R8;
            } else if (components == 3) {   /* red, green, blue */
                format = GL_RGB;
                internalformat = GL_RGB8;
            } else if (components == 4) {  /* red, green, blue, alpha */
                format = GL_RGBA;
                internalformat = GL_RGBA8;
            } else
                fprintf(stderr, "unknown componets: %d\n", components);

            glCreateTextures(GL_TEXTURE_2D, 1, &textureID);
            glTextureParameteri(textureID, GL_TEXTURE_MAX_LEVEL, 0);
            glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureStorage2D(textureID, 1, internalformat, width, height);
            glTextureSubImage2D(textureID, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);

            stbi_image_free(data);
        } else {
            fprintf(stderr, "texture failed to load at path: %s\n", file_name);
            free(file_name);
            exit(-1);
            return;
        }

        texture.id = textureID;
        texture.name = str_alloc(file_name);
        texture.type = type;

        array_push_back(model->textures, texture);
        p = array_back(model->textures);
    }
    else if (p->type != type) {
        texture.id = p->id;
        texture.name = str_alloc(file_name);
        texture.type = type;

        array_push_back(model->textures, texture);
        p = array_back(model->textures);
    }

    array_push_back(m->textures, p);
    free(file_name);
}

static int draw_mesh(model_t *model, mesh_t *mesh, GLuint shader) {
    int i;
    GLuint binding = 0;
    material_t *material = mesh->material;
    texture_t **texture = material->textures;

    for (i = 0; i < (int)array_size(texture); i++) {
        switch (texture[i]->type) {
        case DIFFUSE_TEXTURE:       binding = 0;  break;
        case AMBIENT_TEXTURE:       binding = 1;  break;
        case NORMAL_TEXTURE:        binding = 2;  break;
        case DISPLACEMENT_TEXTURE:  binding = 3;  break;
        case BUMP_TEXTURE:          binding = 4;  break;
        case SPECULAR_TEXTURE:      binding = 5;  break;
        case HEIGHT_TEXTURE:        binding = 6;  break;
        case AO_TEXTURE:            binding = 7;  break;
        case CUBEMAP_TEXTURE:       binding = 8;  break;
        default: fprintf(stderr, " warning: unknown texture type [%d]\n", texture[i]->type); break;
        }
        glBindTextureUnit(binding, texture[i]->id);
    }
    (void)shader;
    (void)model;
    /* draw mesh */
    glBindVertexArray(mesh->vao);
    glNamedBufferSubData(uniform_mtl_handle, 0, sizeof(uniform_mtl), mesh->uniform_mtl);
    glDrawElements(GL_TRIANGLES, array_size(mesh->indices), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    return 0;
}

int draw_obj_model(model_t *model, GLuint shader) {
    mesh_t *mesh = NULL;
    if (NULL == model)  return 0;

    if (array_size(model->meshes) < 1) 
        return -1;

    glNamedBufferSubData(uniform_gui_ctl_handle, 0, sizeof(uniform_gui_ctl), model->uniform_gui);

    /* if draw in wireframe */
    if (model->uniform_gui->polygonMode)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (model->uniform_gui->cullFace)
        glEnable(GL_CULL_FACE);
    else 
        glDisable(GL_CULL_FACE);

    for (mesh = array_begin(model->meshes); mesh != array_end(model->meshes); mesh++) {
        draw_mesh(model, mesh, shader);
    }
    return 0;
}

void model_add_texture(model_t *mdl, char *pic, texture_type type, int texture_should_flip) {
    material_t *mtl = NULL;
    texture_t *tex = NULL;
    if (NULL == mdl || NULL == mdl->meshes || NULL == pic) 
        return;
    if (texture_should_flip)
        stbi_set_flip_vertically_on_load(1);
    load_texture(mdl, NULL, pic, type);
    stbi_set_flip_vertically_on_load(0);
    
    tex = array_back(mdl->textures);

    for (mtl = array_begin(mdl->material); mtl != array_end(mdl->material); mtl++)
        array_push_back(mtl->textures, tex);
    
    update_mesh_mtl_info(mdl);
}

void model_add_texture_with(model_t *mdl, GLuint id, texture_type type, const char* name) {
    texture_t texture;
    material_t *mtl = NULL;
    texture_t *tex = NULL;
    assert(mdl);
    assert(id);
    if (!mdl || !id)
        return;

    if (seach_texture(mdl, name))
        return;

    texture.name = str_alloc(name);
    texture.id = id;
    texture.type = type;
    array_push_back(mdl->textures, texture);
    tex = array_back(mdl->textures);
    for (mtl = array_begin(mdl->material); mtl != array_end(mdl->material); mtl++)
        array_push_back(mtl->textures, tex);

    update_mesh_mtl_info(mdl);
}



void update_mesh_mtl_info(model_t *model) {
    int i;
    mesh_t *mesh = NULL;
    assert(model);
    assert(model->meshes);
    assert(model->material);
    if (!model || !model->meshes || !model->material) 
        return;
    for (mesh = array_begin(model->meshes); mesh != array_end(model->meshes); mesh++) {
        material_t *material = mesh->material;
        texture_t **texture = material->textures;
        assert(mesh->uniform_mtl);
        memset(mesh->uniform_mtl, 0, sizeof(uniform_mtl));
        mesh->uniform_mtl->ambient = to_vec4(material->ambient, 0);
        mesh->uniform_mtl->diffuse = to_vec4(material->ambient, 0);
        mesh->uniform_mtl->specular = to_vec4(material->specular, 0);
        mesh->uniform_mtl->shininess = material->shininess;
        mesh->uniform_mtl->hasTexture = texture == NULL ? YIRAN_FALSE : YIRAN_TRUE;
        for (i = 0; i < (int)array_size(texture); i++) {
            switch (texture[i]->type) {
            case DIFFUSE_TEXTURE:       mesh->uniform_mtl->hasDiffuse = YIRAN_TRUE; break;
            case AMBIENT_TEXTURE:       mesh->uniform_mtl->hasAmbient = YIRAN_TRUE; break;
            case NORMAL_TEXTURE:        mesh->uniform_mtl->hasNormal  = YIRAN_TRUE; break;
            case DISPLACEMENT_TEXTURE:  mesh->uniform_mtl->hasDisp    = YIRAN_TRUE; break;
            case BUMP_TEXTURE:          mesh->uniform_mtl->hasBump    = YIRAN_TRUE; break;
            case SPECULAR_TEXTURE:      mesh->uniform_mtl->hasSpecular= YIRAN_TRUE; break;
            case HEIGHT_TEXTURE:        mesh->uniform_mtl->hasHeight  = YIRAN_TRUE; break;
            case AO_TEXTURE:            mesh->uniform_mtl->hasAO      = YIRAN_TRUE; break;
            case CUBEMAP_TEXTURE:       mesh->uniform_mtl->hasSkybox  = YIRAN_TRUE; break;
            default: fprintf(stderr, " warning: unknown texture type [%d]\n", texture[i]->type); break;
            }
        }
    }
}

void init_uniforms(model_t *model) {
    assert(model);
    if (!model) 
        return;
    model->uniform_gui = (uniform_gui_ctl *)calloc(1, sizeof(uniform_gui_ctl));
    assert(model->uniform_gui);

    if (!model->uniform_gui)    return;
    model->uniform_gui->showAmbient        = 1;
    model->uniform_gui->showDiffuse        = 1;
    model->uniform_gui->showSpec           = 1;

    model->uniform_gui->activeColorMap     = 1;
    model->uniform_gui->activeAmbientMap   = 1;
    model->uniform_gui->activeNormalMap    = 1;
    model->uniform_gui->activeDispMap      = 1;
    model->uniform_gui->activeBumpMap      = 1;
    model->uniform_gui->activeSpecMap      = 1;
    model->uniform_gui->activeHeightMap    = 1;
    model->uniform_gui->activeAoMap        = 1;
    model->uniform_gui->activeCubeMap      = 1; 

    assert(vec3_len(vec3_sub(model->bbox[0], model->bbox[1])) > MAT4_EPSILON);
    model->uniform_gui->bbox[0] = to_vec4(model->bbox[0], 1.0);
    model->uniform_gui->bbox[1] = to_vec4(model->bbox[1], 1.0);
}

