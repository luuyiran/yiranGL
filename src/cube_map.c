#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#include "glad.h"
#include "mat4.h"
#include "cube_map.h"

typedef struct cube_map_t {
    int width;
    int comp;
    void *face[6];
} cube_map_t;

extern float hypotf(float x, float y);
extern unsigned char *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
extern float *stbi_loadf(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
extern void stbi_image_free(void *retval_from_stbi_load);
extern void stbi_set_flip_vertically_on_load(int flag_true_if_should_flip);

static int clamp(int v, int a, int b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

static vec4 get_pixel_float(const float* data, int x, int y, int width, int comp) {
    int ofs = comp * (y * width + x);
    return build_vec4(
            comp > 0 ? data[ofs + 0] : 0.0f, 
            comp > 1 ? data[ofs + 1] : 0.0f,
            comp > 2 ? data[ofs + 2] : 0.0f, 
            comp > 3 ? data[ofs + 3] : 0.0f);
}

static void set_pixel_float(float* data, int x, int y, int width, int comp, vec4 c) {
    int ofs = comp * (y * width + x);
    if (comp > 0) data[ofs + 0] = c.x;
    if (comp > 1) data[ofs + 1] = c.y;
    if (comp > 2) data[ofs + 2] = c.z;
    if (comp > 3) data[ofs + 3] = c.w;
}

static vec3 face_coords_to_xyz(int i, int j, int face_id, int face_size) {
    float A = 2.f * i / face_size;
    float B = 2.f * j / face_size;

    switch (face_id) {
        case 0: return build_vec3(1.f - A, 1.f, 1.f - B);
        case 1: return build_vec3(A - 1.f, -1.f, 1.f - B);
        case 2: return build_vec3(B - 1.f, A - 1.f, 1.f);
        case 3: return build_vec3(1.f - B, A - 1.f, -1.f);
        case 4: return build_vec3(1.f, A - 1.f, 1.f - B);
        case 5: return build_vec3(-1.f, 1-A, 1.f - B);
        
        default:return build_vec3(0.f, 0.f, 0.f);
    }
}

/**
 *  https://stackoverflow.com/questions/29678510/convert-21-equirectangular-panorama-to-cube-map 
 */
static cube_map_t *convert_equirectangular_to_cubemap(const float* hdr, int width, int height, int comp) {
    int i, j, face;
    int U1, V1, U2, V2;
    float R, theta, phi, Uf, Vf, s, t, *result;
    cube_map_t *cube;
    int face_size = width / 4;
    int clampW = width - 1;
    int clampH = height - 1;
    vec3 P;
    vec4 A, B, C, D, color;
    int floats_per_face = face_size * face_size * comp;
    assert(hdr);
    result = malloc(floats_per_face *sizeof(float) * 6);
    assert(result);
    memset(result, 0, floats_per_face *sizeof(float) * 6);

    cube = malloc(sizeof(cube_map_t));
    assert(cube);
    memset(cube, 0, sizeof(cube_map_t));

    cube->width = face_size;
    cube->comp = comp;
    for (i = 0; i < 6; i++)
        cube->face[i] = result + i * floats_per_face;

    for (face = 0; face != 6; face++) {
        for (i = 0; i != face_size; i++) {
            for (j = 0; j != face_size; j++) {
                P = face_coords_to_xyz(i, j, face, face_size);
                R = hypotf(P.x, P.y);
                theta = atan2(P.y, P.x);
                phi = atan2(P.z, R);
                /* float point source coordinates */
                Uf = 2.0f * face_size * (theta + M_PI) / M_PI;
                Vf = 2.0f * face_size * (M_PI / 2.0f - phi) / M_PI;
                /* 4-samples for bilinear interpolation */
                U1 = clamp(floor(Uf), 0, clampW);
                V1 = clamp(floor(Vf), 0, clampH);
                U2 = clamp(U1 + 1, 0, clampW);
                V2 = clamp(V1 + 1, 0, clampH);
                /* fractional part */
                s = Uf - U1;
                t = Vf - V1;
                /* fetch 4-samples */
                A = get_pixel_float(hdr, U1, V1, width, comp);
                B = get_pixel_float(hdr, U2, V1, width, comp);
                C = get_pixel_float(hdr, U1, V2, width, comp);
                D = get_pixel_float(hdr, U2, V2, width, comp);
                /* bilinear interpolation */
                color = vec4_mul_float(A, (1 - s) * (1 - t));
                color = vec4_add(color, vec4_mul_float(B, (s) * (1 - t)));
                color = vec4_add(color, vec4_mul_float(C, (1 - s) * t));
                color = vec4_add(color, vec4_mul_float(D, (s) * (t)));

                set_pixel_float(cube->face[face], i, j, cube->width, cube->comp, color);
            }
        }
    }

    return cube;
}

static void cube_map_free(cube_map_t *cube) {
    if (cube) {
        free(cube->face[0]);
        free(cube);
    }
}


GLuint load_cube_map(const char *face[], int texture_should_flip) {
    int i, w, h, comp;
    GLenum format = GL_RGB;
    GLenum internalformat = GL_RGB8;
    GLuint cubemap_tex;
    unsigned char *data[6] = {NULL};
    if (texture_should_flip)
        stbi_set_flip_vertically_on_load(1);
    for (i = 0; i < 6; i++) {
        data[i] = stbi_load(face[i], &w, &h, &comp, 0);
        assert(data[i]);
    }
    stbi_set_flip_vertically_on_load(0);
    if (comp == 1) { /* gray */
        format = GL_RED;
        internalformat = GL_R8;
    } else if (comp == 3) { /* red, green, blue */
        format = GL_RGB;
        internalformat = GL_RGB8;
    } else if (comp == 4) { /* red, green, blue, alpha */
        format = GL_RGBA;
        internalformat = GL_RGBA8;
    } else
        fprintf(stderr, "unknown componets: %d\n", comp);

    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &cubemap_tex);

    glTextureStorage2D(cubemap_tex, 1, internalformat, w, h);

    for (i = 0; i < 6; i++) {
        glTextureSubImage3D(cubemap_tex, 0, 0, 0, i, w, h, 1, format, GL_UNSIGNED_BYTE, data[i]);
        stbi_image_free(data[i]);
    }

    return cubemap_tex;
}


GLuint load_cube_map_hdr(const char *hdr, int texture_should_flip) {
    float *img = NULL;
    GLenum format = GL_RGB;
    GLenum internalformat = GL_RGB32F;
    cube_map_t *cube = NULL;
    GLuint cubemap_tex = 0;
    int i, w, h, comp;
    if (NULL == hdr)
        return 0;

    if (texture_should_flip)
        stbi_set_flip_vertically_on_load(1);
    img = stbi_loadf(hdr, &w, &h, &comp, 0);
    stbi_set_flip_vertically_on_load(0);
    if (NULL == img) {
        fprintf(stderr, "load_cube_map_hdr: stbi_loadf \"%s\" failed!\n", hdr);
        return 0;
    }

    if (comp == 1) {        /* gray */
        format = GL_RED;
        internalformat = GL_R32F;
    } else if (comp == 3) { /* red, green, blue */
        format = GL_RGB;
        internalformat = GL_RGB32F;
    } else if (comp == 4) { /* red, green, blue, alpha */
        format = GL_RGBA;
        internalformat = GL_RGBA32F;
    } else
        fprintf(stderr, "unknown componets: %d\n", comp);

    cube = convert_equirectangular_to_cubemap(img, w, h, comp);
    stbi_image_free((void*)img);

    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &cubemap_tex);
    glTextureParameteri(cubemap_tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(cubemap_tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(cubemap_tex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTextureParameteri(cubemap_tex, GL_TEXTURE_BASE_LEVEL, 0);
    glTextureParameteri(cubemap_tex, GL_TEXTURE_MAX_LEVEL, 0);
    glTextureParameteri(cubemap_tex, GL_TEXTURE_MAX_LEVEL, 0);
    glTextureParameteri(cubemap_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(cubemap_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureStorage2D(cubemap_tex, 1, internalformat, cube->width, cube->width);


    for (i = 0; i < 6; i++) {
        glTextureSubImage3D(cubemap_tex, 0, 0, 0, i, cube->width, cube->width, 1, format, GL_FLOAT, cube->face[i]);
    }

    cube_map_free(cube);

    return cubemap_tex;
}

void cube_map_delete(unsigned int tex) {
    glDeleteTextures(1, &tex);
}
