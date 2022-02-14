#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "glad.h"
#include "array.h"
#include "model.h"
#include "shader.h"
#include "rbtree.h"
#include "utils.h"
#include "uniforms.h"
#include "draw.h"
#include "teapot_data.h"

typedef struct vertex_dict {
    char        *key;       /* v/vt/vn */
    int          index;     /* index of model_t::vertices */
    rb_node      node;      /* link */
} vertex_dict;

extern void stbi_set_flip_vertically_on_load(int flag_true_if_should_flip);

static void free_vertex_set(rb_node *root) {
    vertex_dict *this = NULL;
    if(NULL == root)
        return;
    free_vertex_set(root->left);
    free_vertex_set(root->right);

    this = RB_TREE_ENTRY(root, vertex_dict, node);
    free(this->key);
    free(this);
}

/* f v1[/vt1][/vn1] v2[/vt2][/vn2] v3[/vt3][/vn3] */
/* Parse triples with index offsets: i, i/j, i/j/k, i//k, */
static void mesh_add_face(model_t *model, mesh_t *m , char *line, vec3 *position, vec2 *coord, vec3 *normal, rb_node **root) {
    int cnt = 0;  /* v count */
    int v  = 0;
    int vt = 0;
    int vn = 0;
    unsigned int index[3] = {0};
    char *pos = NULL;
    char word[128] = {0}; /* one word like v/vt/vn */
    rb_node *parent = NULL;
    rb_node **itor = NULL;
    int result = 0;
    vertex_dict *find = NULL;
    vertex_dict *cur = NULL;
    vertex_dict *n = NULL;
    /* omit 'f' and ' ' */
    while (!is_num(*line)) line++;
 
    while (line && *line) {
        line = get_next_word(word, line);
        pos = &word[0];

        /* search in red-black tree */
        parent = NULL;
        itor = root;
        find = NULL;
        while (*itor) {
            cur = RB_TREE_ENTRY(*itor, vertex_dict, node);
            parent = *itor;
            result = strcmp(word, cur->key);
            if (result < 0)
                itor = &((*itor)->left);
            else if (result > 0)
                itor = &((*itor)->right);
            else {
                find = cur;
                break;
            }
        }

        if (find) {
            index[cnt] = find->index;
        } else {
            /* pos is one of i, i/j, i/j/k, i//k */
            assert(pos);
            v = atoi(pos) - 1;
            pos = after_slash(pos);
            if (*pos) {
                if (*pos != '/')
                    vt =  atoi(pos) - 1;
                pos = after_slash(pos);
                if (*pos)
                    vn =  atoi(pos) - 1;
            }
            
            array_push_back(model->position, position[v]);
            if (coord)
                array_push_back(model->tex_coords, coord[vt]);
            if (normal)
                array_push_back(model->normal, normal[vn]);

            n = (vertex_dict *)malloc(sizeof(vertex_dict));
            assert(n);
            memset(n, 0, sizeof(vertex_dict));
            n->key = str_alloc(word);
            n->index = array_size(model->position) - 1;
            index[cnt] = n->index;
                    
            rb_link_node(parent, &n->node, itor);
            rbt_insert(root, &n->node);
        }

        cnt++;

        if (cnt == 3) {
            array_push_back(m->indices, index[0]);
            array_push_back(m->indices, index[1]);
            array_push_back(m->indices, index[2]);
            /* convert to triangles if the face has more than 3 indices */
            index[1] = index[2];
            cnt = 2;
        }
    }
}

static material_t *seach_material(model_t *model, const char *name) {
    material_t *itor = NULL;
    material_t *material = model->material;

    if (NULL == material)
        return NULL;

    for (itor = array_begin(material); itor != array_end(material); itor++) {
        if (strcmp(itor->name, name) == 0)
            return itor;
    }
    return NULL;
}


static void read_mtl(model_t *model, char *line_buf) {
    char *a = NULL;
    char *head = NULL;
    char *buf = NULL;
    char *mtl = NULL;
    char *line = NULL;
    char *path = NULL;
    material_t material;
    material_t *mtl_p = NULL;
    char word[128] = {0};
    line_buf = get_next_word(word, line_buf);
    line_buf = get_next_word(word, line_buf);

    mtl = path_to_file(model->dir, word);
    path = dir_name(mtl);

    buf = read_file(mtl);
    if (NULL == buf) {
        free(mtl);
        return;
    }

    model->mtl = str_alloc(mtl);
    /* wait for free */
    head = buf;

    line = (char *)malloc(256);
    assert(line);
    a = line;

    memset(&material, 0, sizeof(material_t));
    while (read_line(&buf, line, 255)) {
        line = get_next_word(word, line);
        to_lower(word);
        if (strcmp(word, "newmtl") == 0) {
            get_next_word(word, line);
            array_push_back(model->material, material);
            mtl_p = array_back(model->material);
            mtl_p->name = str_alloc(word);
        } else if (strcmp(word, "ka") == 0) {
            mtl_p->ambient = read_vec3(line);
        } else if (strcmp(word, "kd") == 0) {
            mtl_p->diffuse = read_vec3(line);
        } else if (strcmp(word, "ks") == 0) {
            mtl_p->specular = read_vec3(line);
        } else if (strcmp(word, "ns") == 0) {
            mtl_p->shininess = atof(line);
        } else if (strcmp(word, "map_ka") == 0) {
            get_next_word(word, line);
            load_texture(model, path, word, AMBIENT_TEXTURE);
        } else if (strcmp(word, "map_kd") == 0) {
            get_next_word(word, line);
            load_texture(model, path, word, DIFFUSE_TEXTURE);
        } else if (strcmp(word, "map_ks") == 0) {
            get_next_word(word, line);
            load_texture(model, path, word, SPECULAR_TEXTURE);
        } else if (strcmp(word, "map_bump") == 0 || 
                   strcmp(word, "bump") == 0) {
            get_next_word(word, line);
            load_texture(model, path, word, BUMP_TEXTURE);
        } else if (strcmp(word, "map_kn") == 0 || 
                   strcmp(word, "norm") == 0) {
            get_next_word(word, line);
            load_texture(model, path, word, NORMAL_TEXTURE);
        } else if (strcmp(word, "map_disp") == 0 || 
                   strcmp(word, "disp") == 0) {
            get_next_word(word, line);
            load_texture(model, path, word, DISPLACEMENT_TEXTURE);
        } else {
            /* 
             * fprintf(stderr, "unsupported: %s\n", word); 
             */
        } 
        line = a;
    }

    free(line);
    free(mtl);
    free(head);
    free(path);
}

/**
 * The vertex normals are computed as a weighted average of the surface normals of all the faces the vertex belongs to. 
 * Each weight is proportional to the surface area of the face.
*/
static void re_generate_normals(model_t *model) {
    int i = 0, j = 0, k = 0;
    int size = 0;
    int *visited = NULL;
    int *same_indices = NULL;
    vec3 *pos = NULL;
    mesh_t *msh = NULL;
    unsigned int *indices = NULL;

    float area = 0.f;
    int p0, p1, p2;
    vec3 v0, v1, v2;
    vec3 e0, e1;
    vec3 normal, face_normal;

    if (NULL == model)  return;
    
    if (NULL == model->position) return;
    if (NULL == model->meshes)  return;

    if (model->normal) {
        array_free(model->normal);
        model->normal = NULL;
    }

    pos = model->position;
    size = array_size(pos);

    printf("------ computing the vertex normals ------\n");

    array_resize(model->normal, size, sizeof(vec3));

    for (msh = array_begin(model->meshes); msh != array_end(model->meshes); msh++) {
        indices = msh->indices;
        for (i = 0; i < (int)array_size(indices); i += 3) {
            p0 = indices[i + 0];
            p1 = indices[i + 1];
            p2 = indices[i + 2];
            v0 = pos[p0];
            v1 = pos[p1];
            v2 = pos[p2];
            e0 = vec3_sub(v1, v0);
            e1 = vec3_sub(v2, v0);
            face_normal = vec3_cross(e0, e1);
            normal = vec3_normalize(face_normal);
            area = vec3_len(face_normal) * 0.5f;
            normal = vec3_mul_float(normal, area);

            model->normal[p0] = vec3_add(model->normal[p0], normal);
            model->normal[p1] = vec3_add(model->normal[p1], normal);
            model->normal[p2] = vec3_add(model->normal[p2], normal);
        }
    }

    /* process joint point, they have same position, sum of them */
    visited = (int *)calloc(1, size * sizeof(int));
    same_indices = (int *) calloc(1, size * sizeof(int));
    assert(visited);
    assert(same_indices);
    for (i = 0; i < size - 1; i++) {
        int same_cnt = 0;
        vec3 sum = model->normal[i];
        if (visited[i]) continue;
        visited[i] = YIRAN_TRUE;
        same_indices[same_cnt++] = i;
        for (j = i + 1; j < size; j++) {
            if (visited[j]) continue;
            if (is_same(pos, NULL, i, j)) {
                visited[j] = YIRAN_TRUE;
                same_indices[same_cnt++] = j;
                sum = vec3_add(sum, model->normal[j]);
            }
        }
        if (same_cnt > 1) {
            for (k = 0; k < same_cnt; k++)
                model->normal[same_indices[k]] = sum;
        }
    }

    /* weighted average */
    for (i = 0; i < size; i++)
        model->normal[i] = vec3_normalize(model->normal[i]);

    free(visited);
    free(same_indices);
    printf("------------------------------------------\n");
}

/* right handedness */
static void re_generate_tangents(model_t *model) {
    int i = 0;
    int size = 0;
    vec3 *pos = NULL;
    vec2 *uv = NULL;
    mesh_t *msh = NULL;
    vec3 *tangent1 = NULL;
    vec3 *tangent2 = NULL;
    unsigned int *indices = NULL;

    float area, r;
    int p0, p1, p2;
    vec3 v0, v1, v2;
    vec3 e0, e1;
    vec2 uv0, uv1, uv2;
    float du0, du1, dv0, dv1;
    vec3 tan1, tan2, proj;
    vec3 normal;

    if (NULL == model)  return;
    if (model->tangent)  return;
    if (NULL == model->tex_coords) return;
    if (NULL == model->position) return;
    if (NULL == model->meshes)  return;

    pos = model->position;
    uv = model->tex_coords;
    size = array_size(pos);

    printf("------ computing the vertex tangent ------\n");

    array_resize(tangent1, size, sizeof(vec3));
    array_resize(tangent2, size, sizeof(vec3));
    array_resize(model->tangent, size, sizeof(vec3));

    memset(tangent1, 0, size * sizeof(vec3));
    memset(tangent2, 0, size * sizeof(vec3));
    memset(model->tangent, 0, size * sizeof(vec3));

    for (msh = array_begin(model->meshes); msh != array_end(model->meshes); msh++) {
        indices = msh->indices;
        for (i = 0; i < (int)array_size(indices); i += 3) {
            p0 = indices[i + 0];
            p1 = indices[i + 1];
            p2 = indices[i + 2];

            v0 = pos[p0];
            v1 = pos[p1];
            v2 = pos[p2];

            uv0 = uv[p0];
            uv1 = uv[p1];
            uv2 = uv[p2];

            e0 = vec3_sub(v1, v0);
            e1 = vec3_sub(v2, v0);
            area = 0.5f * vec3_len(vec3_cross(e0, e1));
            
            du0 = uv1.x - uv0.x; 
            du1 = uv2.x - uv0.x;
            dv0 = uv1.y - uv0.y;
            dv1 = uv2.y - uv0.y;
            r = 1.f / (du0 * dv1 - du1 * dv0);
            tan1 = build_vec3((dv1*e0.x - dv0*e1.x) * r,
                              (dv1*e0.y - dv0*e1.y) * r,
                              (dv1*e0.z - dv0*e1.z) * r);
            tan2 = build_vec3((du0*e1.x - du1*e0.x) * r,
                              (du0*e1.y - du1*e0.y) * r,
                              (du0*e1.z - du1*e0.z) * r);

            tan1 = vec3_mul_float(tan1, area);
            tan2 = vec3_mul_float(tan2, area);

            tangent1[p0] = vec3_add(tangent1[p0], tan1);
            tangent1[p1] = vec3_add(tangent1[p1], tan1);
            tangent1[p2] = vec3_add(tangent1[p2], tan1);

            tangent2[p0] = vec3_add(tangent2[p0], tan2);
            tangent2[p1] = vec3_add(tangent2[p1], tan2);
            tangent2[p2] = vec3_add(tangent2[p2], tan2);
        }
    }

    for (i = 0; i < size; i++) {
        normal = model->normal[i];
        tan1 = tangent1[i];
        tan2 = tangent2[i];
        /* Gram-Schmidt orthogonalize */
        proj = vec3_mul_float(normal, vec3_dot(normal, tan1));
        model->tangent[i] = vec3_normalize(vec3_sub(tan1, proj));
    }

    array_free(tangent1);
    array_free(tangent2);

    printf("------------------------------------------\n");
}

static void remove_duplicates(model_t *model) {
    int i = 0;
    int j = 0;
    int size = 0;
    mesh_t *msh = NULL;
    int *index = NULL;
    unsigned int *m_indices = NULL;
    vec3 *pos = NULL;
    vec3 *nor = NULL;
    vec2 *uvs = NULL;
    vec3 *new_pos = NULL;
    vec3 *new_nor = NULL;
    vec2 *new_uvs = NULL;

    if (NULL == model)  return;

    pos = model->position;
    nor = model->normal;
    uvs = model->tex_coords;
    size = array_size(pos);

    if (size == 0) return;

    if (is_unique_vertices(model)) {
        printf("No duplicates vertices!\n");
        return;
    }

    printf("----------- remove_duplicates ------------\n");

    index = (int *)malloc(size * sizeof(int));
    assert(index);
    for (i = 0; i < size; i++)
        index[i] = -1;

    array_reserve(new_pos, size, sizeof(vec3));
    if (nor) array_reserve(new_nor, size, sizeof(vec3));
    if (uvs) array_reserve(new_uvs, size, sizeof(vec2));

    for (i = 0; i < size; i++) {
        if (index[i] == -1) {
            array_push_back(new_pos, pos[i]);
            if (nor) array_push_back(new_nor, nor[i]);
            if (uvs) array_push_back(new_uvs, uvs[i]);

            index[i] = array_size(new_pos) - 1;
        }

        for (j = i + 1; j < size; j++) {
            if (is_same(pos, uvs, i, j)) {
                index[j] = index[i];
            }
        }
    }

    for (msh = array_begin(model->meshes); msh != array_end(model->meshes); msh++) {
        m_indices = msh->indices;
        for (i = 0; i < (int)array_size(m_indices); i++) {
            m_indices[i] = index[m_indices[i]];
        }
    }

    array_free(model->position);
    if (model->normal)      array_free(model->normal);
    if (model->tex_coords)  array_free(model->tex_coords);

    printf("vertices before  :  %d\n", size);
    printf("vertices after   :  %d\n", (int)array_size(new_pos));
    printf("vertices removed :  %d\n", size - (int)array_size(new_pos));

    model->position = new_pos;
    model->normal = new_nor;
    model->tex_coords = new_uvs;
#if 0
    assert(is_unique_vertices(model));
#endif
    printf("------------------------------------------\n");
    free(index);
}

static void model_add_mesh(model_t *model) {
    mesh_t mesh;
    assert(model);
    if (!model) return;
    memset(&mesh, 0, sizeof(mesh_t));
    mesh.uniform_mtl = (uniform_mtl *)calloc(1, sizeof(uniform_mtl));
    assert(mesh.uniform_mtl);
    array_push_back(model->meshes, mesh);
}

static void model_add_default__material(model_t *model) {
    mesh_t *mesh;
    assert(model);
    assert(!model->material && "No need to add a default material!");
    assert(model->meshes && "Must allocate a Mesh first!");

    if (!model || model->material || !model->meshes)
        return;

    printf("adding a default material..\n");

    array_resize(model->material, 1, sizeof(material_t));
    memset(model->material, 0, sizeof(material_t));
    model->mtl = str_alloc("_Default_");
    model->material->name = str_alloc("_Default_Material_");
    model->material->ambient = build_vec3(0.3f, 0.3f, 0.3f);
    model->material->diffuse = build_vec3(1.f, 0.3f, 0.3f);
    model->material->specular = build_vec3(0.5f, 0.5f, 0.5f);
    model->material->shininess = 64.f;
    for (mesh = array_begin(model->meshes); mesh != array_end(model->meshes); mesh++) {
        assert(!mesh->material && "Mesh already have a material!");
        mesh->material = model->material;
    }
}

static void update_bbox(model_t *model) {
    int i, n;
    vec3 *v;
    assert(model);
    assert(model->position);
    if (!model || !model->position) return;

    printf("%s: update bounding box..\n", model->name);
    n = array_size(model->position);
    v = model->position;
    memset(model->bbox, 0, sizeof(model->bbox));
    for (i = 0; i < n; i++) {
        model->bbox[0].x = MIN(model->bbox[0].x, v[i].x);
        model->bbox[0].y = MIN(model->bbox[0].y, v[i].y);
        model->bbox[0].z = MIN(model->bbox[0].z, v[i].z);
        model->bbox[1].x = MAX(model->bbox[1].x, v[i].x);
        model->bbox[1].y = MAX(model->bbox[1].y, v[i].y);
        model->bbox[1].z = MAX(model->bbox[1].z, v[i].z);
    }
}

model_t *load_obj_model(const char *obj_model, int texture_should_flip) {
    int i = 0;
    model_t *model = NULL;
    char *head = NULL;
    char *buf = NULL;
    char line[256] = {0};
    char name[128] = {0};
    char *default_name = "_Default_";
    rb_node *vertex_set = NULL;
    vec3 *v = NULL;
    vec3 *vn = NULL;
    vec2 *vt = NULL;
    vec3 vec3_tmp;
    vec2 vec2_tmp;
    mesh_t *mesh = NULL;

    buf = read_file(obj_model);
    if (NULL == buf) {
        fprintf(stderr, "fopen failed: can't open the model \"%s\".\n", obj_model);
        exit(-1);
    }

    model = (model_t *)calloc(1, sizeof(model_t));
    if (NULL == model) {
        fprintf(stderr, "malloc failed: can't molloc struct model_t!\n");
        exit(-1);
    }
    model->name = str_alloc(obj_model);
    model->dir = dir_name(obj_model);

    /* wait for free */
    head = buf;

    printf("------------------------------------------\n");
    printf("loading \"%s\"...\n", obj_model);
    printf("first pass...\n");
    while (read_line(&buf, line, sizeof(line) - 1)) {
        switch (line[0]) {
            case 'm':
                if (texture_should_flip)
                    stbi_set_flip_vertically_on_load(1);
                read_mtl(model, line);
                stbi_set_flip_vertically_on_load(0);
                break;
            case 'v':
                switch (line[1]) {
                    case ' ':
                        vec3_tmp = read_vec3(line);
                        array_push_back(v, vec3_tmp);
                        break;
                    case 'n':
                        vec3_tmp = read_vec3(line);
                        array_push_back(vn, vec3_tmp);
                        break;
                    case 't':
                        vec2_tmp = read_vec2(line);
                        array_push_back(vt, vec2_tmp);
                        break;
                    default:
                        fprintf(stderr, "can't parse: %s.\n", line);
                        exit(-1);
                }
                break;
            case 'g':
            case 'o':
                model_add_mesh(model);
                break;
            default:
                break;
        }
    }

    /* add a default mesh */
    if (0 == array_size(model->meshes)) {
        printf("adding a default mesh..\n");
        model_add_mesh(model);
        model->meshes[0].name = str_alloc(default_name);
    }

    /* add a default material */
    if (0 == array_size(model->material)) {
        model_add_default__material(model);
    }

    i = 0;
    printf("second pass...\n");
    buf = head;
    mesh = &model->meshes[i];
    while (read_line(&buf, line, sizeof(line) - 1)) {
        switch (line[0]) {
            case 'u':  /* usemtl */
                sscanf(line, "%s %s", name, name);
                mesh->material = seach_material(model, name);
                /* TODO: if NULL,set default material, model->material[0] is default */
                break;
            case 'g':
            case 'o':
                mesh = &model->meshes[i++];
                sscanf(line, "%s %s", name, name);
                mesh->name = str_alloc(name);
                break;
            case 'f':
                mesh_add_face(model, mesh, line, v, vt, vn, &vertex_set);
                break;
            default:
                break;
        }
    }

    if (!model->tex_coords)
        remove_duplicates(model);

    if (NULL == model->normal) {
        printf("generate normals....\n");
        re_generate_normals(model);
    }

    if (model->tex_coords) {
        printf("generate tangents....\n");
        re_generate_tangents(model);
    }

    array_reserve(model->position, array_size(model->position), sizeof(vec3));
    array_reserve(model->normal, array_size(model->normal), sizeof(vec3));
    array_reserve(model->tex_coords, array_size(model->tex_coords), sizeof(vec2));

    update_bbox(model);

    init_uniforms(model);
    update_mesh_mtl_info(model);

    model->scale = 1.f;
    update_model_matrix(model);

    init_buffer(model);

    printf("loaded \"%s\" successfully!\n", obj_model);
    print_model(model);

    free(head);
    if (v)  array_free(v);
    if (vt) array_free(vt);
    if (vn) array_free(vn);
    free_vertex_set(vertex_set);

    return model;
}

mat4 update_model_matrix(model_t *model) {
    mat4 s = scale(model->scale);
    mat4 t = translate(model->translation);
    mat4 rx = rotate_x(model->rotation.x);
    mat4 ry = rotate_y(model->rotation.y);
    mat4 rz = rotate_z(model->rotation.z);
    /* scale -->  rotate --> translate */
    model->model_matrix = mat4_mul_mat4(t, mat4_mul_mat4(rz, mat4_mul_mat4(ry, mat4_mul_mat4(rx, s))));

    return model->model_matrix;
}

void model_translate(model_t *model, vec3 translation) {
    model->translation = translation;
    update_model_matrix(model);
}

void model_scale(model_t *model, float scale) {
    model->scale = scale;
    update_model_matrix(model);
}

void model_rotate_x(model_t *model, float angle) {
    model->rotation.x = angle;
    update_model_matrix(model);
}

void model_rotate_y(model_t *model, float angle) {
    model->rotation.y = angle;
    update_model_matrix(model);
}

void model_rotate_z(model_t *model, float angle) {
    model->rotation.z = angle;
    update_model_matrix(model);
}

void model_rotate(model_t *model, vec3 rotation) {
    model->rotation = rotation;
    update_model_matrix(model);
}

void model_delete(model_t *model) {
    mesh_t *mesh = NULL;
    texture_t *texture = NULL;
    material_t *material = NULL;
    if (NULL == model) return;

    free(model->name);
    free(model->dir);
    free(model->mtl);

    if (model->position)    array_free(model->position);
    if (model->normal)      array_free(model->normal);
    if (model->tangent)     array_free(model->tangent);
    if (model->tex_coords)  array_free(model->tex_coords);

    if (model->meshes) {
        for (mesh = array_begin(model->meshes); mesh != array_end(model->meshes); mesh++) {
            free(mesh->name);
            free(mesh->uniform_mtl);
            array_free(mesh->indices);
            glDeleteBuffers(1, &mesh->ebo);
            glDeleteVertexArrays(1, &mesh->vao);
        }
        array_free(model->meshes);
    }

    if (model->textures) {
        for (texture = array_begin(model->textures); texture != array_end(model->textures); texture++) {
            free(texture->name);
            glDeleteTextures(1, &texture->id);
        }
        array_free(model->textures);
    }

    if (model->material) {
        for (material = array_begin(model->material); material != array_end(model->material); material++) {
            free(material->name);
            array_free(material->textures);
        }
        array_free(model->material);
    }

    if (model->position_buffer) 
        glDeleteBuffers(1, &model->position_buffer);
    if (model->normal_buffer)
        glDeleteBuffers(1, &model->normal_buffer);
    if (model->uv_buffer)
        glDeleteBuffers(1, &model->uv_buffer);
    if (model->tangent_buffer)
        glDeleteBuffers(1, &model->tangent_buffer);

    free(model->uniform_gui);
    free(model);
}

/*
        v5----- v4
       /|      /|
      v1------v0|
      | |     | |
      | |v6---|-|v7
      |/      |/
      v2------v3
*/
#define v0 {1.f, 1.f, 1.f}
#define v1 {-1.f, 1.f, 1.f}
#define v2 {-1.f, -1.f, 1.f}
#define v3 {1.f, -1.f, 1.f}
#define v4 {1.f, 1.f, -1.f}
#define v5 {-1.f, 1.f, -1.f}
#define v6 {-1.f, -1.f, -1.f}
#define v7 {1.f, -1.f, -1.f}

#define front   {0, 0, -1}
#define left    {-1, 0, 0}
#define right   {1, 0, 0}
#define back    {0, 0, 1}
#define top     {0, 1, 0}
#define bottom  {0, -1, 0}

#define uv00 {0,0}
#define uv10 {1,0}
#define uv01 {0,1}
#define uv11 {1,1}

model_t *load_cube_model(void) {
    vec3 v[24] = {       
        /* front */  
        v5, v4, v7, v6, 
        /* left */
        v1, v5, v6, v2,
        /* right */
         v4, v0, v3, v7,
        /* back */
        v0, v1, v2, v3,
        /* top */
        v0, v4, v5, v1,
        /* bottom */
        v3, v2, v6, v7
    };

    vec3 vn[24] = {
        front, front, front, front,
        left, left, left, left,
        right, right, right, right,
        back, back, back, back,
        top, top, top, top,
        bottom, bottom, bottom, bottom
    };

    vec2 vt[24] = {
        uv00, uv10, uv11, uv01, 
        uv00, uv10, uv11, uv01,
        uv00, uv10, uv11, uv01,
        uv00, uv10, uv11, uv01,
        uv00, uv10, uv11, uv01,
        uv00, uv10, uv11, uv01
    };

    unsigned int index[36] = {
        0,1,2,    0,2,3,
        4,5,6,    4,6,7,
        8,9,10,   8,10,11,
        12,13,14, 12,14,15,
        16,17,18, 16,18,19,
        20,21,22, 20,22,23
    };

    model_t *cube = (model_t *)calloc(1, sizeof(model_t));
    if (NULL == cube) {
        fprintf(stderr, "malloc failed: can't molloc struct model_t!\n");
        return NULL;
    }
    model_add_mesh(cube);
    model_add_default__material(cube);
    cube->name = str_alloc("Cube");

    array_resize(cube->position, 24, sizeof(vec3));
    array_resize(cube->normal, 24, sizeof(vec3));
    array_resize(cube->tex_coords, 24, sizeof(vec2));
    array_resize(cube->meshes->indices, 36, sizeof(unsigned int));
    memcpy(cube->position, v, sizeof(v));
    memcpy(cube->normal, vn, sizeof(vn));
    memcpy(cube->tex_coords, vt, sizeof(vt));
    memcpy(cube->meshes->indices, index, sizeof(index));

    re_generate_tangents(cube);

    cube->bbox[0] = build_vec3(-1.f,-1.f,-1.f);
    cube->bbox[1] = build_vec3(1.f,1.f,1.f);

    init_uniforms(cube);
    update_mesh_mtl_info(cube);

    cube->scale = 1.f;
    update_model_matrix(cube);

    init_buffer(cube);

    return cube;
}

model_t *load_sphere_model(float r, int y_edgs) {
    int x, y, idx = 0;
    float dx, dy, two_pi = 2 * M_PI, pi_2 = M_PI / 2;
    vec3 *pos = NULL;
    vec3 *normal = NULL;
    vec3 *tangent = NULL;
    vec2 *uv = NULL;
    GLuint *indices = NULL;
    model_t *sphere = NULL;

    int x_edgs = 2 * y_edgs;
    int n = (x_edgs + 1) * (y_edgs + 1);
    int triangles = 2 * x_edgs * y_edgs;

    assert(r > 0);
    assert(y_edgs > 0);

    sphere = (model_t *)calloc(1, sizeof(model_t));
    assert(sphere);
    if (NULL == sphere) {
        fprintf(stderr, "malloc failed: can't molloc struct model_t!\n");
        return NULL;
    }
    sphere->name = str_alloc("Sphere");
    model_add_mesh(sphere);
    model_add_default__material(sphere);

    array_resize(sphere->position, n, sizeof(vec3));
    array_resize(sphere->normal, n, sizeof(vec3));
    array_resize(sphere->tangent, n, sizeof(vec3));
    array_resize(sphere->tex_coords, n, sizeof(vec2));
    array_resize(sphere->meshes->indices, 3 * triangles, sizeof(GLuint));

    pos     = sphere->position;
    normal  = sphere->normal;
    tangent = sphere->tangent;
    uv      = sphere->tex_coords;
    indices = sphere->meshes->indices;

    dx = two_pi / x_edgs;
    dy = M_PI / y_edgs;

    for (x = 0; x <= x_edgs; x++) {
        float u = x * dx;
        float cu = cos(u);
        float su = sin(u);
        for (y = 0; y <= y_edgs; y++) {
            float v = pi_2 + y * dy;
            float cv = cos(v);
            float sv = sin(v);
            pos[idx] = build_vec3(r * cv * cu, r *sv, r * cv *su);
            normal[idx] = vec3_normalize(pos[idx]);
            tangent[idx] = vec3_normalize(build_vec3(-normal[idx].z, 0, normal[idx].x));   /* right hand */
            uv[idx] = build_vec2(u / two_pi, v / M_PI); 
            idx++;
        }
    }
    assert(idx == n);

    idx = 0;
    for (x = 0; x < x_edgs; x++) {
        int start = x * (y_edgs + 1);
        int next_start = (x + 1) * (y_edgs + 1);
        for (y = 0; y < y_edgs; y++) {
            int next_y = y + 1;
            indices[idx++] = start + y;
            indices[idx++] = next_start + y;
            indices[idx++] = next_start + next_y;

            indices[idx++] = start + y;
            indices[idx++] = next_start + next_y;
            indices[idx++] = start + next_y;
        }
    }

    assert(idx == 3 * triangles);

    sphere->bbox[0] = build_vec3(-r,-r,-r);
    sphere->bbox[1] = build_vec3(r,r,r);


    init_uniforms(sphere);
    update_mesh_mtl_info(sphere);

    sphere->scale = 1.f;
    update_model_matrix(sphere);

    init_buffer(sphere);
    return sphere;
}

model_t *load_torus_model(float R, float r, int nsides, int nrings) {
    int idx = 0, n, ring, side, triangles;
    float dr, ds, two_pi = 2 * M_PI;
    vec3 *pos = NULL;
    vec3 *normal = NULL;
    vec3 *tangent = NULL;
    vec2 *uv = NULL;
    GLuint *indices = NULL;
    model_t *torus = NULL;

    assert(R > 0);
    assert(r > 0);
    assert(R > r);
    assert(nsides > 0);
    assert(nrings > 0);

    /* vertices, One extra to connect uv edge */
    n = (nsides + 1) * (nrings + 1);
    triangles = 2 * nsides * nrings;

    torus = (model_t *)calloc(1, sizeof(model_t));
    assert(torus);
    if (NULL == torus) {
        fprintf(stderr, "malloc failed: can't molloc struct model_t!\n");
        return NULL;
    }
    torus->name = str_alloc("Torus");
    model_add_mesh(torus);
    model_add_default__material(torus);
    
    array_resize(torus->position, n, sizeof(vec3));
    array_resize(torus->normal, n, sizeof(vec3));
    array_resize(torus->tangent, n, sizeof(vec3));
    array_resize(torus->tex_coords, n, sizeof(vec2));
    array_resize(torus->meshes->indices, 3 * triangles, sizeof(GLuint));

    pos     = torus->position;
    normal  = torus->normal;
    tangent = torus->tangent;
    uv      = torus->tex_coords;
    indices = torus->meshes->indices;

    dr = two_pi / nrings;
    ds = two_pi / nsides;

    for (ring = 0; ring <= nrings; ring++) {
        float u = ring * dr;
        float cu = cos(u);
        float su = sin(u);
        for (side = 0; side <= nsides; side++) {
            float v = side * ds;
            float cv = cos(v);
            float sv = sin(v);
            float radius = (R + r * cv);
            pos[idx] = build_vec3(radius * cu, r * sv, radius * su);
            normal[idx] = vec3_normalize(build_vec3(cv * cu, sv, cv * su));
            tangent[idx] = vec3_normalize(build_vec3(-normal[idx].z, 0, normal[idx].x));   /* right hand */
            uv[idx] = build_vec2(u / two_pi, v / two_pi);
            idx++;
        }
    }
    assert(idx == n);

    idx = 0;
    for (ring = 0; ring < nrings; ring++) {
        int start = ring * (nsides + 1);
        int next_start = (ring + 1) * (nsides + 1);
        for (side = 0; side < nsides; side++) {
            int next_side = side + 1;
            indices[idx++] = start + side;
            indices[idx++] = next_start + next_side;
            indices[idx++] = next_start + side;

            indices[idx++] = start + side;
            indices[idx++] = start + next_side;
            indices[idx++] = next_start + next_side;
        }
    }

    assert(idx == 3 * triangles);
    torus->bbox[0] = build_vec3(-R-r, -r, -R-r);
    torus->bbox[1] = build_vec3(R+r, r, R+r);

    init_uniforms(torus);
    update_mesh_mtl_info(torus);

    torus->scale = 1.f;
    update_model_matrix(torus);

    init_buffer(torus);
    return torus;
}


static vec3 point_bezier_at(vec3 *p, float b0, float b1, float b2, float b3) {
    vec3 p0 = vec3_mul_float(p[0], b0);
    vec3 p1 = vec3_mul_float(p[1], b1);
    vec3 p2 = vec3_mul_float(p[2], b2);
    vec3 p3 = vec3_mul_float(p[3], b3);
    return vec3_add(p0, vec3_add(p1, vec3_add(p2, p3)));
}

static vec3 eval_bezier_curve(vec3 *p, float t) {
    float b0 = (1 - t) * (1 - t) * (1 - t);
    float b1 = 3 * t * (1 - t) * (1 - t);
    float b2 = 3 * t * t * (1 - t);
    float b3 = t * t * t;
    return point_bezier_at(p, b0, b1, b2, b3);
}

static vec3 eval_bezier_patch(vec3 *control_points, float u, float v){
    int i;
    vec3 u_curve[4];
    for (i = 0; i < 4; i++)
        u_curve[i] = eval_bezier_curve(control_points + 4 * i, u);
    return eval_bezier_curve(u_curve, v);
}

static vec3 deriv_bezier(vec3 *p, float t) {
    float b0 = -3 * (1 - t) * (1 - t);
    float b1 = 3 * (1 - t) * (1 - t) - 6 * t * (1 - t);
    float b2 = 6 * t * (1 - t) - 3 * t * t;
    float b3 = 3 * t * t;
    return point_bezier_at(p, b0, b1, b2, b3);
}

static vec3 du_bezier(vec3 *ctrl, float u, float v) {
    int i;
    vec3 p[4], v_curve[4];
    for (i = 0; i < 4; ++i) {
        p[0] = ctrl[i];
        p[1] = ctrl[4 + i];
        p[2] = ctrl[8 + i];
        p[3] = ctrl[12 + i];
        v_curve[i] = eval_bezier_curve(p, v);
    }
    return deriv_bezier(v_curve, u);
}

static vec3 dv_bezier(vec3 *ctrl, float u, float v) {
    int i;
    vec3 u_curve[4];
    for (i = 0; i < 4; ++i)
        u_curve[i] = eval_bezier_curve(ctrl + 4 * i, u);
    return deriv_bezier(u_curve, v);
}

model_t *load_teapot_model(int divs) {
    int i, j, k, n, n_per_path, triangles, idx, offset, el;
    vec3 ctrl[16];
    vec3 *pos = NULL;
    vec3 *normal = NULL;
    vec3 *tangent = NULL;
    vec2 *uv = NULL;
    GLuint *indices = NULL;
    model_t *teapot = NULL;

    teapot = (model_t *)calloc(1, sizeof(model_t));
    assert(teapot);
    if (NULL == teapot) {
        fprintf(stderr, "malloc failed: can't molloc struct model_t!\n");
        return NULL;
    }

    n_per_path = (divs + 1) * (divs + 1);
    n = TEAPORT_PATCH_NUMS * n_per_path;
    triangles = 2 * TEAPORT_PATCH_NUMS * divs *divs;

    teapot->name = str_alloc("Teapot");
    model_add_mesh(teapot);
    model_add_default__material(teapot);

    array_resize(teapot->position, n, sizeof(vec3));
    array_resize(teapot->normal, n, sizeof(vec3));
    array_resize(teapot->tangent, n, sizeof(vec3));
    array_resize(teapot->tex_coords, n, sizeof(vec2));
    array_resize(teapot->meshes->indices, 3 * triangles, sizeof(GLuint));

    pos     = teapot->position;
    normal  = teapot->normal;
    tangent = teapot->tangent;
    uv      = teapot->tex_coords;
    indices = teapot->meshes->indices;

    offset = 0;
    idx = 0;
    el = 0;
    for (k = 0; k < TEAPORT_PATCH_NUMS; k++) {
        for (i = 0; i < 16; i++)
            ctrl[i] = teaport_control_point[patch_index[k][i]];

        for (j = 0; j <= divs; j++) {
            float v = (float)j / (float)divs;
            for (i = 0; i <= divs; i++) {
                float u = (float)i / (float)divs;
                vec3 du = du_bezier(ctrl, u, v);
                vec3 dv = dv_bezier(ctrl, u, v);
                vec3 norm = vec3_cross(du, dv);
                if (vec3_len(du) != 0.0)
                    du = vec3_normalize(du);
                if (vec3_len(dv) != 0.0)
                    dv = vec3_normalize(dv);
                if (vec3_len(norm) != 0.0)
                    norm = vec3_normalize(norm);

                pos[idx] = eval_bezier_patch(ctrl, u, v);
                normal[idx] = norm;
                tangent[idx] = du;
                uv[idx] = build_vec2(u, v);
                idx++;
            }
        }

        for (j = 0; j < divs; j++) {
            int start = j * (divs + 1) + offset;
            int start_next = (j + 1) * (divs + 1) + offset;
            for (i = 0; i < divs; i++) {
                int next = i + 1;
                indices[el++] = start_next + next;
                indices[el++] = start_next + i;
                indices[el++] = start + i;

                indices[el++] = start_next + next;
                indices[el++] = start + i;
                indices[el++] = start + next;
            }
            
        }
        offset += n_per_path;
    }

    assert(idx == n);
    assert(el == 3 * triangles);
    update_bbox(teapot);

    init_uniforms(teapot);
    update_mesh_mtl_info(teapot);

    teapot->scale = 1.f;
    update_model_matrix(teapot);

    init_buffer(teapot);

    return teapot;
}

