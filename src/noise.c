#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

#include "glad.h"
#include "mat4.h"
#include "noise.h"



#define TABLE_SIZE  0X100U
#define TABLE_MASK  0XFFU

static vec2 **grid_table = NULL;

static vec2 **get_grid(void) {
    uint32_t i, j;
    if (NULL == grid_table) {
        grid_table = (vec2 **)calloc(TABLE_SIZE, sizeof(vec2 *));
        assert(grid_table);
        for (i = 0; i < TABLE_SIZE; i++) {
            grid_table[i] = (vec2 *)calloc(TABLE_SIZE, sizeof(vec2));
            assert(grid_table[i]);
        }
        srand(time(NULL));
        for (i = 0; i < TABLE_SIZE; i++)
            for (j = 0; j < TABLE_SIZE; j++) 
                grid_table[i][j] = vec2_normalize(build_vec2(rand() % TABLE_SIZE - TABLE_SIZE / 2, rand() % TABLE_SIZE - TABLE_SIZE / 2));
    }
    return grid_table;
}

static float lerpf(float lo, float hi, float t) {
    return lo * (1 - t) + hi * t;
}

static vec2 lerpv(vec2 lo, vec2 hi, float t) {
    float x = lo.x * (1 - t) + hi.x * t;
    float y = lo.y * (1 - t) + hi.y * t;
    return build_vec2(x, y);
}

static float fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

float noise_eval(vec2 p, vec3 *out) {
    uint32_t x = (uint32_t)(p.x);
    uint32_t y = (uint32_t)(p.y);

    uint32_t x0 = x & TABLE_MASK;
    uint32_t x1 = (x0 + 1) & TABLE_MASK;
    uint32_t y0 = y & TABLE_MASK;
    uint32_t y1 = (y0 + 1) & TABLE_MASK;

    float u = fade(p.x - x);
    float v = fade(p.y - y);

    vec2 vec00 = build_vec2(u, v);
    vec2 vec01 = build_vec2(u, 1.f - v);
    vec2 vec10 = build_vec2(1.f - u, v);
    vec2 vec11 = build_vec2(1.f - u, 1.f - v);

    vec2 **grid = get_grid();

    float v00 = vec2_dot(vec00, grid[x0][y0]);
    float v01 = vec2_dot(vec01, grid[x0][y1]);
    float v10 = vec2_dot(vec10, grid[x1][y0]);
    float v11 = vec2_dot(vec11, grid[x1][y1]);
    if (out) {
        vec2 vec = lerpv(lerpv(grid[x0][y0], grid[x1][y0], u),
                        lerpv(grid[x0][y1], grid[x1][y1], u), v);
        out->x = vec.x;
        out->y = vec.y;
        out->z = 1;
    }
    return lerpf(lerpf(v00, v10, u), lerpf(v01, v11, u), v);
}

static vec3 getPixelAt(const uint8_t *data, int x, int y, int width, int height) {
    int ofs;
    if (x < 0) x = width - 1;
    if (x >= width) x = 0;
    if (y < 0) y = height - 1;;
    if (y >= height) y = 0;

    ofs = 3 * (y * width + x);
    return vec3_mul_float(build_vec3(data[ofs + 0], data[ofs + 1], data[ofs + 2]), 1.f / 255.f);
}


void load_noise_vase(GLuint *color_handle, GLuint *normal_handle) {
    int row, col, x, y, idx = 0;
    uint8_t *pixels, *pixelsN;
    float baseFreq = 12;
    int level = 6;
    float amplitudeMult = 0.5f;
    float frequencyMult = 1.9;

    int width = 1024;
    int height = 1024;
    float dx = 1.0f / (width - 1);
    float dy = 1.0f / (height - 1);

    GLuint color_texture, normal_texture;

    assert(color_handle);
    assert(normal_handle);

    pixels = (uint8_t *)malloc(width * height * 3);
    pixelsN = (uint8_t *)malloc(width * height * 3);
    assert(pixels);
    assert(pixelsN);



    for (row = 0; row < height; row++) {
        for (col = 0; col < width / 2; col++) {
            int oct, lofs, rofs;
            float turb;
            float x = dx * col;
            float y = dy * row;
            float freq = baseFreq;
            float amplitude = 1.f;
            vec3 sumVec = build_vec3(0.f, 0.f, 0.f);
            for (oct = 0; oct < level; oct++) {
                float px = freq * x;
                float py = freq * y;
                vec3 val;
                noise_eval(build_vec2(px, py), &val);
                val = vec3_mul_float(val, amplitude);
                sumVec = vec3_add(sumVec, val);
                freq *= frequencyMult;
                amplitude *= amplitudeMult;
            }
            sumVec = vec3_normalize(sumVec);
            turb = sin(sumVec.y);
            lofs = 3 * (row * width + col);
            rofs = 3 * (row * width + width - 1 - col);
            pixels[lofs + 0] = pixels[rofs + 0] = (uint8_t)((turb * 1.0 + 1.0) * 255.f * 0.5);
            pixels[lofs + 1] = pixels[rofs + 1] = (uint8_t)((turb * 0.7 + 1.0) * 255.f * 0.5);
            pixels[lofs + 2] = pixels[rofs + 2] = (uint8_t)((turb * 0.8 + 1.0) * 255.f * 0.5);
        }
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &color_texture);
    glTextureParameteri(color_texture, GL_TEXTURE_MAX_LEVEL, 0);
    glTextureParameteri(color_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(color_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureStorage2D(color_texture, 1, GL_RGB8, width, height);
    glTextureSubImage2D(color_texture, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);


    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            float tl = vec3_len(getPixelAt(pixels, x - 1, y - 1, width, height));
            float t = vec3_len(getPixelAt(pixels, x - 1, y, width, height));
            float tr = vec3_len(getPixelAt(pixels, x - 1, y + 1, width, height));
            float r = vec3_len(getPixelAt(pixels, x, y + 1, width, height));
            float br = vec3_len(getPixelAt(pixels, x + 1, y + 1, width, height));
            float b = vec3_len(getPixelAt(pixels, x + 1, y, width, height));
            float bl = vec3_len(getPixelAt(pixels, x + 1, y - 1, width, height));
            float l = vec3_len(getPixelAt(pixels, x, y - 1, width, height));
            float dx = (tr + 2.f * r + br) - (tl + 2.f * l + bl);
            float dy = (bl + 2.f * b + br) - (tl + 2.f * t + tr);
            float dz = 0.33f;
            vec3 n = vec3_normalize(build_vec3(dx, dy, dz));
            pixelsN[idx++] = (uint8_t)((n.x + 1.0) * 255.f * 0.5);
            pixelsN[idx++] = (uint8_t)((n.y + 1.0) * 255.f * 0.5);
            pixelsN[idx++] = (uint8_t)((n.z + 1.0) * 255.f * 0.5);
        }
    }


    glCreateTextures(GL_TEXTURE_2D, 1, &normal_texture);
    glTextureParameteri(normal_texture, GL_TEXTURE_MAX_LEVEL, 0);
    glTextureParameteri(normal_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(normal_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureStorage2D(normal_texture, 1, GL_RGB8, width, height);
    glTextureSubImage2D(normal_texture, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixelsN);

    *color_handle = color_texture; 
    *normal_handle = normal_texture;

    free(pixels);
    free(pixelsN);
}

