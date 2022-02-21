#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "glad.h"
#include "array.h"
#include "shader.h"
#include "draw.h"
#include "cube_map.h"
#include "render_config.h"
#include "window.h"
#include "model.h"
#include "camera.h"
#include "skybox.h"
#include "uniforms.h"

extern void gui_update(render_config *cfg);
extern void gui_render(struct nk_gui_t* gui);
extern void gui_init(render_config *cfg);
extern void nk_gui_scroll_callback(struct nk_gui_t* gui, double yoff);
extern void nk_gui_char_callback(struct nk_gui_t* gui, unsigned int codepoint);
extern void nk_gui_mouse_button_callback(window_t* win, button_t button, int action);
extern int  nk_gui_is_any_hovered(window_t* win);
extern void nk_gui_shutdown(struct nk_gui_t* gui);

static void init_uniform_handles(void) {
    glCreateBuffers(1, &uniform_matrix_handle);
    glNamedBufferStorage(uniform_matrix_handle, sizeof(uniform_matrix), NULL, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, uniform_matrix_handle, 0, sizeof(uniform_matrix));

    glCreateBuffers(1, &uniform_mtl_handle);
    glNamedBufferStorage(uniform_mtl_handle, sizeof(uniform_mtl), NULL, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferRange(GL_UNIFORM_BUFFER, 1, uniform_mtl_handle, 0, sizeof(uniform_mtl));

    glCreateBuffers(1, &uniform_gui_ctl_handle);
    glNamedBufferStorage(uniform_gui_ctl_handle, sizeof(uniform_gui_ctl), NULL, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferRange(GL_UNIFORM_BUFFER, 2, uniform_gui_ctl_handle, 0, sizeof(uniform_gui_ctl));
}

static void uniform_buffer_destroy(void) {
    glDeleteBuffers(1, &uniform_matrix_handle);
    glDeleteBuffers(1, &uniform_mtl_handle);
    glDeleteBuffers(1, &uniform_gui_ctl_handle);
}

render_config *config_init(int width, int height) {
    render_config *cfg = (render_config *)calloc(1, sizeof(render_config));
    assert(cfg);
    cfg->width   = width;
    cfg->height  = height;
    return cfg;
}

void config_delete(render_config *cfg) {
    int i = 0;
    shader_config *shader = NULL;
    if (NULL == cfg)    return;
    for (i = 0; i < (int)array_size(cfg->models); i++)
        model_delete(cfg->models[i]);

    skybox_delete(cfg->skybox);
    camera_delete(cfg->camera);

    for (shader = array_begin(cfg->shaders); shader != array_end(cfg->shaders); shader++)
        shader_delete(shader->shader);

    array_free(cfg->models);
    array_free(cfg->shaders);
    window_terminate(cfg->window);  
    nk_gui_shutdown(cfg->gui);
    uniform_buffer_destroy();
    free(cfg);
}

void config_add_model(render_config *cfg, model_t *m) {
    if (NULL == cfg || NULL == m)  return;
    array_push_back(cfg->models, m);
}

void config_set_camera(render_config *cfg, camera_t *cmr) {
    if (NULL == cfg || NULL == cmr)  return;
    if (cfg->camera)
        camera_delete(cfg->camera);
    cfg->camera = cmr;
}

void config_set_skybox(render_config *cfg, GLuint cube) {
    int i;
    assert(cfg);
    assert(cfg->models && "should load a model first.");
    if (NULL == cfg || 0 == cube)   return;

    if (cfg->skybox)
        skybox_delete(cfg->skybox);

    cfg->skybox = skybox_create(cube);
    assert(cfg->skybox);
    if (!cfg->skybox)
        return;

    for (i = 0; i < (int)array_size(cfg->models); i++)
        model_add_texture_with(cfg->models[i], cube, CUBEMAP_TEXTURE, "_YIRAN_CUBE_MAP_");
}

void config_add_shader(render_config *cfg, GLuint shader, SET_UNIFORMS set) {
    shader_config config;
    config.shader = shader;
    config.set_matrix = set;
    array_push_back(cfg->shaders, config);
}

void config_set_window(render_config *cfg, window_t *window) {
    if (NULL == cfg || NULL == window)  return;
    if (cfg->window)
        window_terminate(window);
    cfg->window = window;
}

static void framebuffer_size_callback(window_t* window, int width, int height) {
    camera_t *camera = NULL;
    render_config *cfg = (render_config *)window_get_user_pointer(window);
    if (NULL == window || NULL == cfg || NULL == cfg->camera)
        return;
    camera = cfg->camera;
    window_size(cfg->window, &width, &height);
    cfg->width = width;
    cfg->height= height;
    camera->aspect_ratio = (float)width / (float)height;
    camera->projection = perspective(camera->zoom, camera->aspect_ratio, camera->near, camera->far);
    glViewport(0, 0, width, height);
}

static void cursor_callback(window_t* window, double xpos, double ypos) {
    static bool firstMouse = true;
    static float lastx, lasty;
    float xoffset = 0.f;
    float yoffset = 0.f;
    int state = YIRAN_RELEASE;
    render_config *cfg = (render_config *)window_get_user_pointer(window);
    if (NULL == window || NULL == cfg || NULL == cfg->camera)
        return;

    if (nk_gui_is_any_hovered(window))
        return;

    state = is_mouse_button_pressed(BUTTON_LEFT);
    if (state == YIRAN_RELEASE) {
        firstMouse = true;
        return;
    }
    if (firstMouse) {
        lastx = xpos;
        lasty = ypos;
        firstMouse = false;
        return;
    }

    xoffset = xpos - lastx;
    yoffset = lasty - ypos;

    lastx = xpos;
    lasty = ypos;

    camera_update_mouse(cfg->camera, xoffset, yoffset);
}

static void scroll_callback(window_t* window, double yoffset) {
    render_config *cfg = (render_config *)window_get_user_pointer(window);
    if (NULL == window || NULL == cfg || NULL == cfg->camera)
        return;
    
    if (nk_gui_is_any_hovered(window))
        nk_gui_scroll_callback(cfg->gui, yoffset);
    else 
        camera_update_scroll(cfg->camera, yoffset);
}


static void char_callback(window_t *window, unsigned int codepoint) {
    if (nk_gui_is_any_hovered(window)) {
        render_config *cfg = (render_config *)window_get_user_pointer(window);
        nk_gui_char_callback(cfg->gui, codepoint);
    }
}

static void process_keyboard(render_config *cfg) {
    if (NULL == cfg || NULL == cfg->camera)
        return;

    if (nk_gui_is_any_hovered(cfg->window))
        return;

    if (is_key_pressed(KEY_W) || is_key_pressed(KEY_UP))
        camera_update_keyboard(cfg->camera, FORWARD, cfg->delta_time);
    if (is_key_pressed(KEY_S) || is_key_pressed(KEY_DOWN))
        camera_update_keyboard(cfg->camera, BACKWARD, cfg->delta_time);
    if (is_key_pressed(KEY_A) || is_key_pressed(KEY_LEFT))
        camera_update_keyboard(cfg->camera, LEFT, cfg->delta_time);
    if (is_key_pressed(KEY_D) || is_key_pressed(KEY_RIGHT))
        camera_update_keyboard(cfg->camera, RIGHT, cfg->delta_time);
}

static void gpu_info() {
    GLint major, minor, nExtensions, nrAttributes;
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *vendor = glGetString(GL_VENDOR);
    const GLubyte *version = glGetString(GL_VERSION);
    const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    glGetIntegerv(GL_NUM_EXTENSIONS, &nExtensions);

    printf("GL Vendor             : %s\n", vendor);
    printf("GL Renderer           : %s\n", renderer);
    printf("GL Version (string)   : %s\n", version);
    printf("GL Version (integer)  : %d.%d\n", major, minor);
    printf("GLSL Version          : %s\n", glslVersion);
    printf("Supported %d attributes.\n", nrAttributes);
    printf("Supported %d extensions.\n", nExtensions);
/**
 *   printf("-------------------------\n");
 *   for( int i = 0; i < nExtensions; i++ )
 *       printf("%s\n", glGetStringi(GL_EXTENSIONS, i));
 *   printf("-------------------------\n");
 */
}

void config_window(render_config *cfg, const char *title) {
    window_t* window = NULL;
    if (NULL == cfg)    return;
    window = window_create(cfg->width, cfg->height, title);
    if (window == NULL) {
        fprintf(stderr, "Failed to create window!\n");
        window_terminate(window);
        return;
    }
    cfg->window = window;
    cfg->vsynch = window_get_swap_interval();

    window_set_user_pointer(window, cfg);

    window_set_framebuffer_size_callback(window, framebuffer_size_callback);
    window_set_button_callback(window, nk_gui_mouse_button_callback);
    window_set_cursor_pos_callback(window, cursor_callback);
    window_set_scroll_callback(window, scroll_callback);
    window_set_char_callback(window, char_callback);

    if (!gladLoadGL()) {
        fprintf(stderr, "GLAD: Loader OpenGL Functions Failed!\n");
        return;
    }

    init_uniform_handles();
    gui_init(cfg);

    gpu_info();
}

void config_shader_matrix(render_config *cfg, mat4 modelMatrix) {
    uniform_matrix matrix;
    matrix.modelViewMatrix = mat4_mul_mat4(cfg->camera->view, modelMatrix);
    matrix.projectionMatrix= cfg->camera->projection;
    matrix.normalMatrix    = transposed(inverse(mat4_mul_mat4(cfg->camera->view, modelMatrix)));
    matrix.viewportMatrix  = view_port_matrix(cfg->width, cfg->height);
    matrix.MVP = mat4_mul_mat4(matrix.projectionMatrix, matrix.modelViewMatrix);
    matrix.cameraPos = to_vec4(cfg->camera->position, 1.f);
    glNamedBufferSubData(uniform_matrix_handle, 0, sizeof(uniform_matrix), &matrix);
}

void  render_loop(render_config *cfg) {
    int i = 0;
    int vsync_state = 1;
    int first_frame = 1;
    shader_config *shader = NULL;
    double current_frame, last_frame, tik, tok;

    GLint available = 0;
    GLuint gpu_query_app_start = 0;
    GLuint gpu_query_app_stop = 0;
    GLuint gpu_query_gui_start = 0;
    GLuint gpu_query_gui_stop = 0;

    GLuint64 gpu_time_start = 0;
    GLuint64 gpu_time_end = 0;

    if (NULL == cfg)            return;
    if (NULL == cfg->window)    return;
    if (NULL == cfg->models)    return;
    if (NULL == cfg->camera)    return;
    if (NULL == cfg->shaders)   return;
    assert(uniform_matrix_handle);
    assert(uniform_mtl_handle);
    assert(uniform_gui_ctl_handle);

    glGenQueries(1, &gpu_query_app_start);
    glGenQueries(1, &gpu_query_app_stop);
    glGenQueries(1, &gpu_query_gui_start);
    glGenQueries(1, &gpu_query_gui_stop);

    current_frame = window_get_time(cfg->window);
    last_frame = current_frame;

    glPatchParameteri(GL_PATCH_VERTICES, 3);

    while (window_is_open(cfg->window)) {
        current_frame = window_get_time(cfg->window);
        cfg->delta_time = current_frame - last_frame;
        last_frame = current_frame;
        if (!first_frame) {
            int fps = (int)(1.0 / cfg->delta_time);
            if (abs(fps - cfg->fps) > 3)
                cfg->fps = (int)(1.0 / cfg->delta_time);
        }

        tik = window_get_time(cfg->window);
        window_poll_events();
        process_keyboard(cfg);
        tok = window_get_time(cfg->window);
        cfg->event_time_cpu = 1000 * (tok - tik);

        tik = window_get_time(cfg->window);
        gui_update(cfg);
        tok = window_get_time(cfg->window);
        cfg->gui_time_cpu = 1000 * (tok - tik);

        /* vertical synchronization */
        if (cfg->vsynch != vsync_state) {
            vsync_state = cfg->vsynch;
            window_set_swap_interval(cfg->vsynch);
        }

        if (!first_frame) {
            glGetQueryObjectiv(gpu_query_app_stop, GL_QUERY_RESULT, &available);
            while (!available) {
                fprintf(stderr, "GPU: Waiting on app GPU timer!\n");
                glGetQueryObjectiv(gpu_query_app_stop, GL_QUERY_RESULT, &available);
            }
            glGetQueryObjectui64v(gpu_query_app_start, GL_QUERY_RESULT, &gpu_time_start);
            glGetQueryObjectui64v(gpu_query_app_stop, GL_QUERY_RESULT, &gpu_time_end);
            cfg->app_time_gpu = (double)(gpu_time_end - gpu_time_start) / 1000000.0;
        }


        glQueryCounter(gpu_query_app_start, GL_TIMESTAMP);

        glEnable(GL_DEPTH_TEST);
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        for (shader = array_begin(cfg->shaders); shader != array_end(cfg->shaders); shader++) {
            shader_use(shader->shader);
            for (i = 0; i < (int)array_size(cfg->models); i++) {
                shader->set_matrix(cfg, cfg->models[i]->model_matrix);
                draw_obj_model(cfg->models[i], shader->shader);
            }
        }

        if (cfg->skybox)
            skybox_render(cfg, cfg->skybox);
        
        glQueryCounter(gpu_query_app_stop, GL_TIMESTAMP);
        

        if (!first_frame) {
            glGetQueryObjectiv(gpu_query_gui_stop, GL_QUERY_RESULT, &available);
            while (!available) {
                fprintf(stderr, "Waiting on GUI GPU timer!\n");
                glGetQueryObjectiv(gpu_query_gui_stop, GL_QUERY_RESULT, &available);
            }
            glGetQueryObjectui64v(gpu_query_gui_start, GL_QUERY_RESULT, &gpu_time_start);
            glGetQueryObjectui64v(gpu_query_gui_stop, GL_QUERY_RESULT, &gpu_time_end);
            cfg->gui_time_gpu = (double)(gpu_time_end - gpu_time_start) / 1000000.0;
        }

        glQueryCounter(gpu_query_gui_start, GL_TIMESTAMP);
        gui_render(cfg->gui);
        glQueryCounter(gpu_query_gui_stop, GL_TIMESTAMP);

        tik = window_get_time(cfg->window);
        window_swap_bufers(cfg->window);
        if (window_get_swap_interval())
            glFinish();     /* block until all GL execution is complete */
        tok = window_get_time(cfg->window);
        cfg->swap_time_wait = 1000 * (tok - tik);
        
        first_frame = 0;
    }
}
