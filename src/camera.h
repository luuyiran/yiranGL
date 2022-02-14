#ifndef __CAMERA_H__
#define __CAMERA_H__
#include "mat4.h"

#define FORWARD     1
#define BACKWARD    2
#define LEFT        4
#define RIGHT       8

#define ORBIT           0
#define FIRST_PERSON    1


typedef struct camera_t {
    int  type;
    vec3 position;
    vec3 target;

    vec3 front;
    vec3 up;
    vec3 right;

    float near;
    float far;
    float zoom; /* fov */
    float aspect_ratio;

    float speed;
    float sensitivity;

    mat4 projection, view;
} camera_t;

void camera_update_keyboard(camera_t *camera, int direction, float deltaTime);
void camera_update_mouse(camera_t *camera, float xoffset, float yoffset);
void camera_update_scroll(camera_t *camera, float yoffset);

camera_t *camera_create(int type, vec3 position, vec3 target, float asp);
float *camera_projection(camera_t *camera);
float *camera_view(camera_t *camera);
void camera_delete(camera_t *camera);

#endif


