#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable : 28251)
#pragma warning(disable : 28159)
#pragma warning(disable : 26812)
#pragma warning(disable : 4116)

#pragma warning(disable : 6385)
#pragma warning(disable : 6001)
#pragma warning(disable : 6386)
#pragma warning(disable : 6011)
#pragma warning(disable : 28182)
#endif 

#if 1
#define NK_IMPLEMENTATION
#endif 
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_KEYSTATE_BASED_INPUT

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <time.h>
#include <limits.h>
#include <math.h>
#include "nuklear.h"
#include "glad.h"
#include "window.h"
#include "render_config.h"
#include "array.h"
#include "camera.h"
#include "model.h"
#include "uniforms.h"

#define NK_SHADER_VERSION  "#version 450 core\n"
#define MAX_VERTEX_BUFFER   512 * 1024
#define MAX_ELEMENT_BUFFER  128 * 1024

#ifndef NK_GUI_TEXT_MAX
#define NK_GUI_TEXT_MAX     256
#endif
#ifndef NK_DOUBLE_CLICK_LO
#define NK_DOUBLE_CLICK_LO  0.02
#endif
#ifndef NK_DOUBLE_CLICK_HI
#define NK_DOUBLE_CLICK_HI  0.2
#endif

struct nk_device_t {
    struct nk_buffer cmds;
    struct nk_draw_null_texture null;
    GLuint vbo, vao, ebo;
    GLuint prog;
    GLuint vert_shdr;
    GLuint frag_shdr;
    GLint attrib_pos;
    GLint attrib_uv;
    GLint attrib_col;
    GLint uniform_tex;
    GLint uniform_proj;
    GLuint font_tex;
};

struct nk_gui_t {
    int width, height;
    int display_width, display_height;
    struct nk_device_t *ogl;
    struct nk_context *ctx;
    struct nk_font_atlas *atlas;
    struct nk_vec2 fb_scale;
    unsigned int text[NK_GUI_TEXT_MAX];
    int text_len;
    struct nk_vec2 scroll;
    double last_button_click;
    int is_double_click_down;
    struct nk_vec2 double_click_pos;
};

struct nk_gui_vertex {
    float position[2];
    float uv[2];
    nk_byte col[4];
};

NK_INTERN void
nk_gui_device_create(struct nk_gui_t* gui) {
    GLint status;
    static const GLchar *vertex_shader =
        NK_SHADER_VERSION
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 TexCoord;\n"
        "in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main() {\n"
        "   Frag_UV = TexCoord;\n"
        "   Frag_Color = Color;\n"
        "   gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
        "}\n";
    static const GLchar *fragment_shader =
        NK_SHADER_VERSION
        "precision mediump float;\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main(){\n"
        "   Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    struct nk_device_t *dev = gui->ogl;
    nk_buffer_init_default(&dev->cmds);
    dev->prog = glCreateProgram();
    dev->vert_shdr = glCreateShader(GL_VERTEX_SHADER);
    dev->frag_shdr = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(dev->vert_shdr, 1, &vertex_shader, 0);
    glShaderSource(dev->frag_shdr, 1, &fragment_shader, 0);
    glCompileShader(dev->vert_shdr);
    glCompileShader(dev->frag_shdr);
    glGetShaderiv(dev->vert_shdr, GL_COMPILE_STATUS, &status);
    assert(status == GL_TRUE);
    glGetShaderiv(dev->frag_shdr, GL_COMPILE_STATUS, &status);
    assert(status == GL_TRUE);
    glAttachShader(dev->prog, dev->vert_shdr);
    glAttachShader(dev->prog, dev->frag_shdr);
    glLinkProgram(dev->prog);
    glGetProgramiv(dev->prog, GL_LINK_STATUS, &status);
    assert(status == GL_TRUE);

    dev->uniform_tex = glGetUniformLocation(dev->prog, "Texture");
    dev->uniform_proj = glGetUniformLocation(dev->prog, "ProjMtx");
    dev->attrib_pos = glGetAttribLocation(dev->prog, "Position");
    dev->attrib_uv = glGetAttribLocation(dev->prog, "TexCoord");
    dev->attrib_col = glGetAttribLocation(dev->prog, "Color");

    {
        /* buffer setup */
        GLsizei vs = sizeof(struct nk_gui_vertex);
        size_t vp = offsetof(struct nk_gui_vertex, position);
        size_t vt = offsetof(struct nk_gui_vertex, uv);
        size_t vc = offsetof(struct nk_gui_vertex, col);

        glGenBuffers(1, &dev->vbo);
        glGenBuffers(1, &dev->ebo);
        glGenVertexArrays(1, &dev->vao);

        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        glEnableVertexAttribArray((GLuint)dev->attrib_pos);
        glEnableVertexAttribArray((GLuint)dev->attrib_uv);
        glEnableVertexAttribArray((GLuint)dev->attrib_col);

        glVertexAttribPointer((GLuint)dev->attrib_pos, 2, GL_FLOAT, GL_FALSE, vs, (void*)vp);
        glVertexAttribPointer((GLuint)dev->attrib_uv, 2, GL_FLOAT, GL_FALSE, vs, (void*)vt);
        glVertexAttribPointer((GLuint)dev->attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void*)vc);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

NK_INTERN void
nk_gui_device_upload_atlas(struct nk_gui_t* gui, const void *image, int width, int height) {
    struct nk_device_t *dev = gui->ogl;
    glGenTextures(1, &dev->font_tex);
    glBindTexture(GL_TEXTURE_2D, dev->font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image);
}

NK_INTERN void
nk_gui_device_destroy(struct nk_gui_t* gui) {
    struct nk_device_t *dev = gui->ogl;
    glDetachShader(dev->prog, dev->vert_shdr);
    glDetachShader(dev->prog, dev->frag_shdr);
    glDeleteShader(dev->vert_shdr);
    glDeleteShader(dev->frag_shdr);
    glDeleteProgram(dev->prog);
    glDeleteTextures(1, &dev->font_tex);
    glDeleteBuffers(1, &dev->vbo);
    glDeleteBuffers(1, &dev->ebo);
    nk_buffer_free(&dev->cmds);
}

NK_INTERN void
nk_gui_render(struct nk_gui_t* gui, enum nk_anti_aliasing AA, int max_vertex_buffer, int max_element_buffer) {
    struct nk_device_t *dev = gui->ogl;
    struct nk_buffer vbuf, ebuf;
    GLfloat ortho[4][4] = {
        {2.0f, 0.0f, 0.0f, 0.0f},
        {0.0f,-2.0f, 0.0f, 0.0f},
        {0.0f, 0.0f,-1.0f, 0.0f},
        {-1.0f,1.0f, 0.0f, 1.0f},
    };
    ortho[0][0] /= (GLfloat)gui->width;
    ortho[1][1] /= (GLfloat)gui->height;

    /* setup global state */
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    /* setup program */
    glUseProgram(dev->prog);
    glUniform1i(dev->uniform_tex, 0);
    glUniformMatrix4fv(dev->uniform_proj, 1, GL_FALSE, &ortho[0][0]);
    glViewport(0,0,(GLsizei)gui->display_width,(GLsizei)gui->display_height);
    {
        /* convert from command queue into draw list and draw to screen */
        const struct nk_draw_command *cmd;
        void *vertices, *elements;
        const nk_draw_index *offset = NULL;

        /* allocate vertex and element buffer */
        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        glBufferData(GL_ARRAY_BUFFER, max_vertex_buffer, NULL, GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, max_element_buffer, NULL, GL_STREAM_DRAW);

        /* load draw vertices & elements directly into vertex + element buffer */
        vertices = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        elements = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        {
            /* fill convert configuration */
            struct nk_convert_config config;
            static const struct nk_draw_vertex_layout_element vertex_layout[] = {
                {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_gui_vertex, position)},
                {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_gui_vertex, uv)},
                {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_gui_vertex, col)},
                {NK_VERTEX_LAYOUT_END}
            };
            memset(&config, 0, sizeof(config));
            config.vertex_layout = vertex_layout;
            config.vertex_size = sizeof(struct nk_gui_vertex);
            config.vertex_alignment = NK_ALIGNOF(struct nk_gui_vertex);
            config.null = dev->null;
            config.circle_segment_count = 22;
            config.curve_segment_count = 22;
            config.arc_segment_count = 22;
            config.global_alpha = 1.0f;
            config.shape_AA = AA;
            config.line_AA = AA;

            /* setup buffers to load vertices and elements */
            nk_buffer_init_fixed(&vbuf, vertices, (size_t)max_vertex_buffer);
            nk_buffer_init_fixed(&ebuf, elements, (size_t)max_element_buffer);
            nk_convert(gui->ctx, &dev->cmds, &vbuf, &ebuf, &config);
        }
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

        /* iterate over and execute each draw command */
        nk_draw_foreach(cmd, gui->ctx, &dev->cmds)
        {
            if (!cmd->elem_count) continue;
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor(
                (GLint)(cmd->clip_rect.x * gui->fb_scale.x),
                (GLint)((gui->height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)) * gui->fb_scale.y),
                (GLint)(cmd->clip_rect.w * gui->fb_scale.x),
                (GLint)(cmd->clip_rect.h * gui->fb_scale.y));
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
            offset += cmd->elem_count;
        }
        nk_clear(gui->ctx);
        nk_buffer_clear(&dev->cmds);
    }

    /* default OpenGL state */
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
}

NK_API void
nk_gui_char_callback(struct nk_gui_t* gui, unsigned int codepoint) {
    if (gui->text_len < NK_GUI_TEXT_MAX)
        gui->text[gui->text_len++] = codepoint;
}

NK_API void
nk_gui_scroll_callback(struct nk_gui_t* gui, double yoff) {
    gui->scroll.y += (float)yoff;
}

NK_API nk_bool
nk_gui_is_any_hovered(window_t* win) {
    render_config *cfg = (render_config *)window_get_user_pointer(win);
    struct nk_gui_t* gui = cfg->gui;
    return nk_window_is_any_hovered(gui->ctx);
}

NK_API void
nk_gui_mouse_button_callback(window_t* win, button_t button, int action) {
    render_config *cfg = (render_config *)window_get_user_pointer(win);
    struct nk_gui_t* gui = cfg->gui;
    double x, y;
    if (button != BUTTON_LEFT) return;
    window_get_cursor_pos(win, &x, &y);
    if (action == YIRAN_PRESS)  {
        double dt = window_get_time(win) - gui->last_button_click;
        if (dt > NK_DOUBLE_CLICK_LO && dt < NK_DOUBLE_CLICK_HI) {
            gui->is_double_click_down = nk_true;
            gui->double_click_pos = nk_vec2((float)x, (float)y);
        }
        gui->last_button_click = window_get_time(win);
    } else gui->is_double_click_down = nk_false;

}

NK_INTERN void
nk_gui_font_stash_begin(struct nk_gui_t* gui) {
    nk_font_atlas_init_default(gui->atlas);
    nk_font_atlas_begin(gui->atlas);
}

NK_INTERN void
nk_gui_font_stash_end(struct nk_gui_t* gui) {
    const void *image; int w, h;
    image = nk_font_atlas_bake(gui->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    nk_gui_device_upload_atlas(gui, image, w, h);
    nk_font_atlas_end(gui->atlas, nk_handle_id((int)gui->ogl->font_tex), &gui->ogl->null);
    if (gui->atlas->default_font)
        nk_style_set_font(gui->ctx, &gui->atlas->default_font->handle);
}

NK_INTERN void
nk_gui_new_frame(render_config *cfg) {
    int i;
    double x, y;
    struct nk_gui_t* gui = cfg->gui;
    struct nk_context *ctx = gui->ctx;
    struct window_t *win = cfg->window;

    window_size(win, &gui->width, &gui->height);
    window_frame_buffer_size(win, &gui->display_width, &gui->display_height);
    gui->fb_scale.x = (float)gui->display_width/(float)gui->width;
    gui->fb_scale.y = (float)gui->display_height/(float)gui->height;

    nk_input_begin(ctx);
    for (i = 0; i < gui->text_len; ++i)
        nk_input_unicode(ctx, gui->text[i]);

    nk_input_key(ctx, NK_KEY_DEL, is_key_pressed(KEY_DELETE) );
    nk_input_key(ctx, NK_KEY_ENTER, is_key_pressed(KEY_ENTER) );
    nk_input_key(ctx, NK_KEY_TAB, is_key_pressed(KEY_TAB) );
    nk_input_key(ctx, NK_KEY_BACKSPACE, is_key_pressed(KEY_BACKSPACE) );
    nk_input_key(ctx, NK_KEY_UP, is_key_pressed(KEY_UP) );
    nk_input_key(ctx, NK_KEY_DOWN, is_key_pressed(KEY_DOWN) );
    nk_input_key(ctx, NK_KEY_TEXT_START, is_key_pressed(KEY_HOME) );
    nk_input_key(ctx, NK_KEY_TEXT_END, is_key_pressed(KEY_END) );
    nk_input_key(ctx, NK_KEY_SCROLL_START, is_key_pressed(KEY_HOME) );
    nk_input_key(ctx, NK_KEY_SCROLL_END, is_key_pressed(KEY_END) );
    nk_input_key(ctx, NK_KEY_SCROLL_DOWN, is_key_pressed(KEY_PAGE_DOWN) );
    nk_input_key(ctx, NK_KEY_SCROLL_UP, is_key_pressed(KEY_PAGE_UP) );
    nk_input_key(ctx, NK_KEY_SHIFT, is_key_pressed(KEY_LEFT_SHIFT) ||
                                    is_key_pressed(KEY_RIGHT_SHIFT) );

    if (is_key_pressed(KEY_LEFT_CONTROL)  ||
        is_key_pressed(KEY_RIGHT_CONTROL) ) {
        nk_input_key(ctx, NK_KEY_COPY, is_key_pressed(KEY_C) );
        nk_input_key(ctx, NK_KEY_PASTE, is_key_pressed(KEY_V) );
        nk_input_key(ctx, NK_KEY_CUT, is_key_pressed(KEY_X) );
        nk_input_key(ctx, NK_KEY_TEXT_UNDO, is_key_pressed(KEY_Z) );
        nk_input_key(ctx, NK_KEY_TEXT_REDO, is_key_pressed(KEY_R) );
        nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, is_key_pressed(KEY_LEFT) );
        nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, is_key_pressed(KEY_RIGHT) );
        nk_input_key(ctx, NK_KEY_TEXT_LINE_START, is_key_pressed(KEY_B) );
        nk_input_key(ctx, NK_KEY_TEXT_LINE_END, is_key_pressed(KEY_E) );
    } else {
        nk_input_key(ctx, NK_KEY_LEFT, is_key_pressed(KEY_LEFT) );
        nk_input_key(ctx, NK_KEY_RIGHT, is_key_pressed(KEY_RIGHT) );
        nk_input_key(ctx, NK_KEY_COPY, 0);
        nk_input_key(ctx, NK_KEY_PASTE, 0);
        nk_input_key(ctx, NK_KEY_CUT, 0);
        nk_input_key(ctx, NK_KEY_SHIFT, 0);
    }

    window_get_cursor_pos(win, &x, &y);
    nk_input_motion(ctx, (int)x, (int)y);

    nk_input_button(ctx, NK_BUTTON_LEFT, (int)x, (int)y, is_mouse_button_pressed(BUTTON_LEFT));
    nk_input_button(ctx, NK_BUTTON_MIDDLE, (int)x, (int)y, is_mouse_button_pressed(BUTTON_MIDDLE));
    nk_input_button(ctx, NK_BUTTON_RIGHT, (int)x, (int)y, is_mouse_button_pressed(BUTTON_RIGHT));
    nk_input_button(ctx, NK_BUTTON_DOUBLE, (int)gui->double_click_pos.x, (int)gui->double_click_pos.y, gui->is_double_click_down);
    nk_input_scroll(ctx, gui->scroll);
    nk_input_end(gui->ctx);
    gui->text_len = 0;
    gui->scroll = nk_vec2(0,0);
}

NK_API
void nk_gui_shutdown(struct nk_gui_t* gui) {
    nk_font_atlas_clear(gui->atlas);
    nk_free(gui->ctx);
    nk_gui_device_destroy(gui);
    memset(gui, 0, sizeof(*gui));
    free(gui->ctx);
    free(gui->ogl);
    free(gui->atlas);
    free(gui);
}

/***************************************************************************************/

NK_API
void gui_init(render_config *cfg) {
    struct nk_gui_t *gui = (struct nk_gui_t *)calloc(1, sizeof(struct nk_gui_t));
    struct nk_context *ctx = (struct nk_context *)calloc(1, sizeof(struct nk_context));
    struct nk_device_t *ogl = (struct nk_device_t *)calloc(1, sizeof(struct nk_device_t));
    struct nk_font_atlas *atlas = (struct nk_font_atlas *)calloc(1, sizeof(struct nk_font_atlas));
    assert(gui);
    assert(cfg);
    assert(ctx);

    gui->ctx = ctx;
    gui->ogl = ogl;
    gui->atlas = atlas;

    nk_init_default(ctx, NULL);
    gui->last_button_click = 0;
    nk_gui_device_create(gui);

    gui->is_double_click_down = nk_false;
    gui->double_click_pos = nk_vec2(0, 0);

    nk_gui_font_stash_begin(gui);
    nk_gui_font_stash_end(gui);

    cfg->gui = gui;
}


NK_INTERN float 
scale_map_to_ui(float s) {
    static float sra = 1.f / 9.f;
    if (s <= 1)
        return s - 1;
    else if (s > 1)
        return sra * s - sra;
    else 
        return 0.f;
}

NK_INTERN float 
scale_map(float s_ui) {
    if (s_ui <= 0) 
        return s_ui + 1;
    else if (s_ui > 0)
        return 9 * s_ui + 1;
    else 
        return 1.f;
}

NK_API
void gui_update(render_config *cfg) {
    struct nk_gui_t* gui = cfg->gui;
    struct nk_context *ctx = gui->ctx;
    camera_t *camera = cfg->camera;
    char buf[128];

    nk_gui_new_frame(cfg);

    if (nk_begin(ctx, "Panel", nk_rect(0, 0, 350, cfg->height), NK_WINDOW_BORDER|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE)) {
        nk_layout_row_dynamic(ctx, 15, 1);

        nk_checkbox_label(ctx, "vsynch", &cfg->vsynch);

        sprintf(buf, "fps       : %d", cfg->fps); 
        nk_label(ctx, buf, NK_TEXT_LEFT);

        sprintf(buf, "time      : %.3f ms", 1000 * cfg->delta_time); 
        nk_label(ctx, buf, NK_TEXT_LEFT);
        sprintf(buf, "event cpu : %.3f ms", cfg->event_time_cpu); 
        nk_label(ctx, buf, NK_TEXT_LEFT);
        sprintf(buf, "app gpu   : %.3f ms", cfg->app_time_gpu); 
        nk_label(ctx, buf, NK_TEXT_LEFT);
        sprintf(buf, "gui cpu   : %.3f ms", cfg->gui_time_cpu); 
        nk_label(ctx, buf, NK_TEXT_LEFT);
        sprintf(buf, "gui gpu   : %.3f ms", cfg->gui_time_gpu); 
        nk_label(ctx, buf, NK_TEXT_LEFT);
        sprintf(buf, "swap wait : %.3f ms", cfg->swap_time_wait); 
        nk_label(ctx, buf, NK_TEXT_LEFT);

        if (nk_tree_push(ctx, NK_TREE_TAB, "Camera", NK_MINIMIZED)) {
            nk_layout_row_dynamic(ctx, 15, 1);

            sprintf(buf, "type    : %s", camera->type == ORBIT ? "Orbit" : "First Person"); 
            nk_label(ctx, buf, NK_TEXT_LEFT);

            sprintf(buf, "position: (%.3f, %.3f, %.3f)", camera->position.x, camera->position.y, camera->position.z);
            nk_label(ctx, buf, NK_TEXT_LEFT);
            sprintf(buf, "target  : (%.3f, %.3f, %.3f)", camera->target.x, camera->target.y, camera->target.z);
            nk_label(ctx, buf, NK_TEXT_LEFT);

            sprintf(buf, "near    : %.3f", camera->near);
            nk_label(ctx, buf, NK_TEXT_LEFT);

            sprintf(buf, "far     : %.3f", camera->far);
            nk_label(ctx, buf, NK_TEXT_LEFT);

            sprintf(buf, "fov     : %.3f°", camera->zoom);
            nk_label(ctx, buf, NK_TEXT_LEFT);

            nk_layout_row_static(ctx, 15, 200, 1);
            nk_property_float(ctx, "speed      :", 0.1, &camera->speed, 1000.f, 0.1f, 0.2f);
            nk_property_float(ctx, "sensitivity:", 0.1, &camera->sensitivity, 1000.f, 0.1f, 0.1f);

            nk_tree_pop(ctx);
        }

        if (nk_tree_push(ctx, NK_TREE_TAB, "Models", NK_MINIMIZED)) {
            int i = 0;
            texture_t *t = NULL;
            model_t *mdl = NULL;

            for (i = 0; i < (int)array_size(cfg->models); i++) {
                mdl = cfg->models[i];
                if (nk_tree_push(ctx, NK_TREE_TAB, mdl->name, NK_MINIMIZED)) {

                    if (nk_tree_push(ctx, NK_TREE_TAB, "Scale", NK_MINIMIZED)) {
                        float s = mdl->scale;
                        float s_ui = scale_map_to_ui(s);
                        nk_layout_row_dynamic(ctx, 15, 1);

                        sprintf(buf, "scale : %.3f", s);
                        nk_label(ctx, buf, NK_TEXT_LEFT);
                        nk_slider_float(ctx, -0.99f, &s_ui, 1.f, 0.01f);
                        mdl->scale = scale_map(s_ui);

                        nk_tree_pop(ctx);
                    }

                    if (nk_tree_push(ctx, NK_TREE_TAB, "Position", NK_MINIMIZED)) {
                        nk_layout_row_dynamic(ctx, 15, 2);

                        sprintf(buf, "x : %.3f", mdl->translation.x);
                        nk_label(ctx, buf, NK_TEXT_LEFT);
                        nk_slider_float(ctx, -10, &mdl->translation.x, 10.0, 0.1f);

                        sprintf(buf, "y : %.3f", mdl->translation.y);
                        nk_label(ctx, buf, NK_TEXT_LEFT);
                        nk_slider_float(ctx, -10, &mdl->translation.y, 10.0, 0.1f);

                        sprintf(buf, "z : %.3f", mdl->translation.z);
                        nk_label(ctx, buf, NK_TEXT_LEFT);
                        nk_slider_float(ctx, -10, &mdl->translation.z, 10.0, 0.1f);

                        nk_tree_pop(ctx);
                    }

                    if (nk_tree_push(ctx, NK_TREE_TAB, "Rotation", NK_MINIMIZED)) {
                        nk_layout_row_dynamic(ctx, 15, 2);

                        sprintf(buf, "x : %.3f", mdl->rotation.x);
                        nk_label(ctx, buf, NK_TEXT_LEFT);
                        nk_slider_float(ctx, -180, &mdl->rotation.x, 180.0, 0.1f);

                        sprintf(buf, "y : %.3f", mdl->rotation.y);
                        nk_label(ctx, buf, NK_TEXT_LEFT);
                        nk_slider_float(ctx, -180, &mdl->rotation.y, 180.0, 0.1f);

                        sprintf(buf, "z : %.3f", mdl->rotation.z);
                        nk_label(ctx, buf, NK_TEXT_LEFT);
                        nk_slider_float(ctx, -180, &mdl->rotation.z, 180.0, 0.1f);

                        nk_tree_pop(ctx);
                    }

                    update_model_matrix(mdl);

                    if (nk_tree_push(ctx, NK_TREE_TAB, "Blinn Phong", NK_MINIMIZED)) {
                        nk_layout_row_dynamic(ctx, 15, 3);
                        nk_checkbox_label(ctx, "ambient", &mdl->uniform_gui->showAmbient);
                        nk_checkbox_label(ctx, "diffuse", &mdl->uniform_gui->showDiffuse);
                        nk_checkbox_label(ctx, "specular", &mdl->uniform_gui->showSpec);
                        nk_tree_pop(ctx);
                    }

                    if (nk_tree_push(ctx, NK_TREE_TAB, "Bbox/WireFrame/Normal/Tangent", NK_MINIMIZED)) {
                        nk_layout_row_static(ctx, 15, 100, 2);
                        nk_checkbox_label(ctx, "Bbox", &mdl->uniform_gui->showBbox);
                        nk_checkbox_label(ctx, "WireFrame", &mdl->uniform_gui->showWireframe);
                        nk_checkbox_label(ctx, "LineMode", &mdl->uniform_gui->polygonMode);
                        nk_checkbox_label(ctx, "CullBack", &mdl->uniform_gui->cullFace);

                        nk_layout_row_static(ctx, 30, 80, 3);
                        mdl->uniform_gui->showNormal = nk_option_label(ctx, "Normal", mdl->uniform_gui->showNormal == 1) ? 1 : mdl->uniform_gui->showNormal;
                        mdl->uniform_gui->showNormal = nk_option_label(ctx, "Tangent", mdl->uniform_gui->showNormal == 2) ? 2 : mdl->uniform_gui->showNormal;
                        mdl->uniform_gui->showNormal = nk_option_label(ctx, "None", mdl->uniform_gui->showNormal == 0) ? 0 : mdl->uniform_gui->showNormal;

                        nk_tree_pop(ctx);
                    }

                    if (nk_tree_push(ctx, NK_TREE_TAB, "Texture", NK_MINIMIZED)) {
                        nk_layout_row_dynamic(ctx, 15, 1);
                        for (t = array_begin(mdl->textures); t != array_end(mdl->textures); t++) {
                            nk_bool *active = NULL;
                            switch (t->type) {
                                case DIFFUSE_TEXTURE:        
                                    sprintf(buf, "color   : %s", t->name);  
                                    active = &mdl->uniform_gui->activeColorMap;     
                                    break;
                                case SPECULAR_TEXTURE:       
                                    sprintf(buf, "specular: %s", t->name);  
                                    active = &mdl->uniform_gui->activeSpecMap;     
                                    break;
                                case AMBIENT_TEXTURE:        
                                    sprintf(buf, "ambient : %s", t->name);  
                                    active = &mdl->uniform_gui->activeAmbientMap;     
                                    break;
                                case NORMAL_TEXTURE:         
                                    sprintf(buf, "normal  : %s", t->name);  
                                    active = &mdl->uniform_gui->activeNormalMap;     
                                    break;
                                case BUMP_TEXTURE:           
                                    sprintf(buf, "bump    : %s", t->name);  
                                    active = &mdl->uniform_gui->activeBumpMap;     
                                    break;
                                case DISPLACEMENT_TEXTURE:   
                                    sprintf(buf, "disp    : %s", t->name);  
                                    active = &mdl->uniform_gui->activeDispMap;     
                                    break;
                                case AO_TEXTURE:             
                                    sprintf(buf, "ssao    : %s", t->name);  
                                    active = &mdl->uniform_gui->activeAoMap;     
                                    break;
                                case CUBEMAP_TEXTURE:
                                    sprintf(buf, "cubemap : %s", t->name);  
                                    active = &mdl->uniform_gui->activeCubeMap;     
                                    break;
                                default:                     
                                    buf[0] = 0;
                                    active  = NULL;
                                    break;
                            }
                            nk_checkbox_label(ctx, buf, active);
                        }
                        nk_tree_pop(ctx);
                    }
                    nk_tree_pop(ctx);
                }
            }
            nk_tree_pop(ctx);
        }
    }
    nk_end(ctx);
}

NK_API
void gui_render(struct nk_gui_t* gui) {
    nk_gui_render(gui, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
}
