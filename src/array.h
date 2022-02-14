#ifndef __YIRAN_ARRAY_H__
#define __YIRAN_ARRAY_H__

#define BUF_HEAD(ptr)   ((size_t *)(ptr) - 2)

#define array_free(ptr)         free(ptr == NULL ? NULL : BUF_HEAD(ptr))
#define array_size(ptr)         (ptr == NULL ? 0 : (BUF_HEAD(ptr)[0]))
#define array_capacity(ptr)     (ptr == NULL ? 0 : (BUF_HEAD(ptr)[1]))

#define array_begin(ptr)     (ptr)
#define array_end(ptr)       (ptr + array_size(ptr))
#define array_back(ptr)      (ptr == NULL ? NULL : (array_end(ptr) - 1))

#define array_push_back(ptr, val)               do_array_push_back((char **)&ptr, &val, sizeof(val))
#define array_reserve(ptr, capacity, item_len)  do_array_reserve((char **)&ptr, capacity, item_len)
#define array_resize(ptr, size, item_len)       do_array_resize((char **)&ptr, size, item_len)

void do_array_push_back(char **ptr, void *src, size_t item_len);
void do_array_reserve(char **ptr, size_t capacity, size_t item_len);
void do_array_resize(char **ptr, size_t size, size_t item_len);


/*
 * ----------------.EXAMPLE -------------------------
 * int main() {
 *      // make sure initialized for NULL.
 *      _Ty *a = NULL;
 *      {
 *          _Ty item;
 *          // construct item
 *          array_push_back(a, item);
 *      }
 *      array_free(a);
 * }
 * 
 * --------------------------------------------------
 */


#endif /* !__YIRAN_ARRAY_H__ */
