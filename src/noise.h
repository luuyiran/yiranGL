#ifndef __NOISE_H__
#define __NOISE_H__

struct vec3;
float noise_eval(vec2 p, struct vec3 *out);

void load_noise_vase(unsigned int *color_handle, unsigned int *normal_handle);

#endif
