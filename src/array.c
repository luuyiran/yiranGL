#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "array.h"

#define ARRAY_INIT_SIZE 128

void do_array_push_back(char **ptr, void *src, size_t item_len) {
    size_t *buf = NULL;

    assert(ptr);
    if (*ptr == NULL) {
        size_t capacity = ARRAY_INIT_SIZE;
        buf = (size_t *)calloc(1, capacity * item_len + 2 * sizeof(size_t));
        assert(buf);
        buf[0] = 0;         /* size */
        buf[1] = capacity;  /* capacity */
        *ptr = (char *)&buf[2];
    } else {
        buf = BUF_HEAD(*ptr);
        if (buf[0] == buf[1]) {
            buf[1] *= 1.5;
            buf = (size_t *)realloc(buf, item_len * buf[1] + 2 * sizeof(size_t));
            assert(buf);
            *ptr = (char *)&buf[2];
        }
    }
    memcpy(*ptr + item_len * buf[0], src, item_len);
    buf[0]++;
}

void do_array_reserve(char **ptr, size_t capacity, size_t item_len) {
    size_t *buf = NULL;
    if (capacity == 0)  return;
    assert(ptr);
    if (*ptr == NULL) {
        buf = (size_t *)calloc(1, capacity * item_len + 2 * sizeof(size_t));
        assert(buf);
        buf[0] = 0;
        buf[1] = capacity;
        *ptr = (char *)&buf[2];
    } else {
        buf = BUF_HEAD(*ptr);
        if (capacity <= buf[1])
            return;
        buf[1] = capacity;
        buf = (size_t *)realloc(buf, item_len * buf[1] + 2 * sizeof(size_t));
        assert(buf);
        *ptr = (char *)&buf[2];
    }
}

void do_array_resize(char **ptr, size_t size, size_t item_len) {
    size_t *buf = NULL;
    if (size == 0)  return;
    assert(ptr);
    assert(size);
    do_array_reserve(ptr, size, item_len);
    assert(*ptr);
    buf = BUF_HEAD(*ptr);
    buf[0] = size;
}

