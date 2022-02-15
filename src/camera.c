#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "camera.h"


static void update_projection(camera_t *camera) {
    if (NULL == camera) return;
    camera->projection = perspective(camera->zoom, camera->aspect_ratio, camera->near, camera->far);
}

static void update_view(camera_t *camera) {
    if (NULL == camera) return;
    camera->view = lookAt(camera->position, camera->target, camera->up);
}

void camera_update_keyboard(camera_t *camera, int direction, float deltaTime) {
    float velocity = 0.f;
    vec3 move_forward;
    vec3 move_right;
    if (NULL == camera) return;
    velocity = camera->speed * deltaTime;
    move_forward = vec3_mul_float(camera->front, velocity);
    move_right = vec3_mul_float(camera->right, velocity);

    if (camera->type == FIRST_PERSON) {
        if (direction == FORWARD) {
            camera->position = vec3_add(camera->position, move_forward);
            camera->target = vec3_add(camera->target, move_forward);
        } else if (direction == BACKWARD) {
            camera->position = vec3_sub(camera->position, move_forward);
            camera->target = vec3_sub(camera->target, move_forward);
        } else if (direction == RIGHT) {
            camera->position = vec3_add(camera->position, move_right);
            camera->target = vec3_add(camera->target, move_right);
        } else if (direction == LEFT) {
            camera->position = vec3_sub(camera->position, move_right);
            camera->target = vec3_sub(camera->target, move_right);
        } else {
            printf("[FIRST_PERSON]camera direction type error: %d.\n", direction); 
        }
    } else if (camera->type == ORBIT) {
        if (direction == FORWARD) {
            if (vec3_len(vec3_sub(camera->target, camera->position)) > vec3_len(move_forward))
                camera->position = vec3_add(camera->position, move_forward);
        } else if (direction == BACKWARD) {
            camera->position = vec3_sub(camera->position, move_forward);
        } 
    }
    update_view(camera);
}



void camera_update_mouse(camera_t *camera, float xoffset, float yoffset) {
    quat qx, qy;
    float angle;
    vec3 world_up = build_vec3(0.f, 1.f, 0.f);
    if (NULL == camera) return;

    xoffset *= camera->sensitivity;
    yoffset *= camera->sensitivity;

    camera->front = vec3_normalize(vec3_sub(camera->target, camera->position));
    camera->right = vec3_normalize(vec3_cross(camera->front, world_up));
    camera->up = vec3_normalize(vec3_cross(camera->right, camera->front));

    angle = DEGREES(acos(vec3_dot(camera->front, world_up)));

    if (camera->type == ORBIT) {
        if ((yoffset > 0 && (angle - yoffset < 20)) || (yoffset < 0 && (angle - yoffset > 160)))
            return;
        qx = quat_angle_axis(-xoffset, camera->up);
        camera->position = vec3_add(quat_rotate(qx, vec3_sub(camera->position, camera->target)), camera->target);
        camera->front = vec3_normalize(vec3_sub(camera->target, camera->position));
        camera->right = vec3_normalize(vec3_cross(camera->front, camera->up));

        qy = quat_angle_axis(yoffset, camera->right);
        camera->position = vec3_add(quat_rotate(qy, vec3_sub(camera->position, camera->target)), camera->target);
        camera->front = vec3_normalize(vec3_sub(camera->target, camera->position));
        camera->up = vec3_normalize(vec3_cross(camera->right, camera->front));
    } else {
        if ((yoffset < 0 && (angle + yoffset < 20)) || (yoffset > 0 && (angle + yoffset > 160)))
            return;
        qx = quat_angle_axis(xoffset, camera->up);
        camera->target = vec3_add(quat_rotate(qx, vec3_sub(camera->target, camera->position)), camera->position);
        camera->front = vec3_normalize(vec3_sub(camera->target, camera->position));
        camera->right = vec3_normalize(vec3_cross(camera->front, camera->up));

        qy = quat_angle_axis(-yoffset, camera->right);
        camera->target = vec3_add(quat_rotate(qy, vec3_sub(camera->target, camera->position)), camera->position);
        camera->front = vec3_normalize(vec3_sub(camera->target, camera->position));
        camera->up = vec3_normalize(vec3_cross(camera->right, camera->front));  
    }
    update_view(camera);
}

void camera_update_scroll(camera_t *camera, float yoffset) {
    if (NULL == camera) return;
    if (camera->zoom + yoffset > 120 || camera->zoom + yoffset < 20)
        return;

    camera->zoom += yoffset * camera->sensitivity;
    update_projection(camera);

    return;
}


camera_t *camera_create(int type, vec3 position, vec3 target, float asp) {
    vec3 world_up = build_vec3(0.f, 1.f, 0.f);
    camera_t *c = (camera_t *)malloc(sizeof(camera_t));
    if (NULL == c) return NULL;

    c->type = type;
    c->position = position;
    c->target = target;
    c->front = vec3_normalize(vec3_sub(c->target, c->position));
    c->right = vec3_normalize(vec3_cross(c->front, world_up));
    c->up = vec3_normalize(vec3_cross(c->right, c->front));
    c->near = 0.1f;
    c->far = 10000.f;
    c->zoom = 45.f;
    c->aspect_ratio = asp;
    c->speed = vec3_len(vec3_sub(c->target, c->position)) * 5.f;
    c->sensitivity = 1.f;
    update_projection(c);
    update_view(c);
    return c;
}

void camera_delete(camera_t *camera) {
    if (camera) free(camera);
}

float *camera_projection(camera_t *camera) {
    if (NULL == camera) return NULL;
    return (float *)&camera->projection;
}

float *camera_view(camera_t *camera) {
    if (NULL == camera) return NULL;
    return (float *)&camera->view;
}


