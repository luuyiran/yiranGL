#include <assert.h>
#include <stdio.h>
#include "glad.h"

#include "cube_map.h"
#include "shader.h"
#include "render_config.h"
#include "camera.h"
#include "draw.h"
#include "uniforms.h"


int WIDTH = 1280;
int HEIGHT= 720;


#define USE_HDR 1


int main(int argc, char *argv[]) {
    float z = 0.f;
    camera_t *camera = NULL;
    model_t *mdl = NULL;
    render_config *cfg = NULL;
    vec3 minb, maxb, target;
    GLuint cube_tex, main_shader, normal_display;
    
    /*
    char *file = "models/stone/Earth3.obj";
    */
   char *file = "models/sponza/sponza.obj";
   
#if USE_HDR
    char *hdr = "hdr/piazza_bologni_1k.hdr";
#else

    const char *faces[6] = {
        "skybox/right.jpg",     /* +x - GL_TEXTURE_CUBE_MAP_POSITIVE_X */
        "skybox/left.jpg",      /* -x - GL_TEXTURE_CUBE_MAP_NEGATIVE_X */
        "skybox/top.jpg",       /* +y - GL_TEXTURE_CUBE_MAP_POSITIVE_Y */
        "skybox/bottom.jpg",    /* -y - GL_TEXTURE_CUBE_MAP_NEGATIVE_Y */
        "skybox/front.jpg",     /* +z - GL_TEXTURE_CUBE_MAP_POSITIVE_Z */
        "skybox/back.jpg"       /* -z - GL_TEXTURE_CUBE_MAP_NEGATIVE_Z */
    };
#endif

    if (argc > 1)
        file = argv[1];

    cfg = config_init(WIDTH, HEIGHT);
    assert(cfg);
    config_window(cfg, "yiranGL");

	main_shader = shader_create("shaders/yiranGL.vert", "shaders/yiranGL.frag", "shaders/yiranGL.geom");
    /* main_shader = shader_create("shaders/test/normalmap_0.vert", "shaders/test/normalmap_0.frag", NULL); */
    normal_display = shader_create("shaders/display_util.vert", "shaders/display_util.frag", "shaders/display_util.geom");


#if 0
    mdl = load_obj_model(file, 1);
    assert(mdl);
    config_add_model(cfg, mdl);
#endif 
#if 0
    mdl = load_obj_model("models/spot/spot_quadrangulated.obj", 1);
    assert(mdl);
    config_add_model(cfg, mdl);

#endif 
#if 0
    mdl = load_torus_model(5, 1.5, 16, 32);
    model_add_texture(mdl, "models/stone/BaseColor.png", DIFFUSE_TEXTURE, 0);
    model_add_texture(mdl, "models/stone/Normal.png", NORMAL_TEXTURE, 0);
    model_add_texture(mdl, "models/stone/Height.png", BUMP_TEXTURE, 0);
    config_add_model(cfg, mdl);
#endif 
#if 0
    mdl = load_cube_model();
    model_add_texture(mdl, "models/stone/BaseColor.png", DIFFUSE_TEXTURE, 0);
    model_add_texture(mdl, "models/stone/Normal.png", NORMAL_TEXTURE, 0);
    model_add_texture(mdl, "models/stone/Height.png", BUMP_TEXTURE, 0);
    config_add_model(cfg, mdl);
#endif 
#if 1
    mdl = load_sphere_model(2, 16);
    model_add_texture(mdl, "models/stone/BaseColor.png", DIFFUSE_TEXTURE, 0);
    model_add_texture(mdl, "models/stone/Normal.png", NORMAL_TEXTURE, 0);
    model_add_texture(mdl, "models/stone/Height.png", BUMP_TEXTURE, 0);
    config_add_model(cfg, mdl);
#endif 
#if 0
    mdl = load_teapot_model(16);
    model_translate(mdl, build_vec3(1, 0, 0));
    model_add_texture(mdl, "models/stone/BaseColor.png", DIFFUSE_TEXTURE, 0);
    model_add_texture(mdl, "models/stone/Normal.png", NORMAL_TEXTURE, 0);

    config_add_model(cfg, mdl);
#endif 

#if 1
    mdl = load_vase_model(64);
    model_add_texture(mdl, "models/stone/BaseColor.png", DIFFUSE_TEXTURE, 0);
    model_add_texture(mdl, "models/stone/Normal.png", NORMAL_TEXTURE, 0);
    model_add_texture(mdl, "models/stone/Height.png", BUMP_TEXTURE, 0);
    config_add_model(cfg, mdl);
#endif 



    minb = mdl->bbox[0];
    maxb = mdl->bbox[1];
    z = maxb.z + 2 * (maxb.z - minb.z);
    target = vec3_mul_float(vec3_add(minb, maxb), 0.5f);
#if 1
    camera = camera_create(ORBIT, build_vec3(0, 0, z), target, (float)WIDTH / (float)HEIGHT);
#else 
    camera = camera_create(FIRST_PERSON, build_vec3(0, 20, 50), target, (float)WIDTH / (float)HEIGHT);
#endif 
    assert(camera);
    config_set_camera(cfg, camera);

#if 1
#if USE_HDR
    cube_tex = load_cube_map_hdr(hdr, 0);
#else
    cube_tex = load_cube_map(faces, 0);
#endif 
    assert(cube_tex);
    config_set_skybox(cfg, cube_tex);
#endif


    config_add_shader(cfg, main_shader, config_shader_matrix);
    config_add_shader(cfg, normal_display, config_shader_matrix);


    render_loop(cfg);

    config_delete(cfg);

    return 0;
}