#ifndef WINDOW_H__
#define WINDOW_H__

#define YIRAN_FALSE     0
#define YIRAN_TRUE	    1

#define YIRAN_RELEASE   0
#define YIRAN_PRESS     1

typedef enum {
    KEY_A,  KEY_B,  KEY_C,  KEY_D,  
    KEY_E,  KEY_S,  KEY_R,  KEY_V,  
    KEY_W,  KEY_X,  KEY_Z,
    
    KEY_LEFT,           KEY_RIGHT,
    KEY_UP,             KEY_DOWN,
    KEY_DELETE,         KEY_ENTER,
    KEY_TAB,            KEY_BACKSPACE,
    KEY_SPACE,          KEY_END,          
    KEY_HOME,           KEY_PAGE_DOWN,    
    KEY_PAGE_UP,        KEY_LEFT_SHIFT, 
    KEY_RIGHT_SHIFT,    KEY_LEFT_CONTROL, 
    KEY_RIGHT_CONTROL,  KEY_COUNT
} keycode_t;

typedef enum { 
    BUTTON_LEFT, 
    BUTTON_RIGHT, 
    BUTTON_MIDDLE, 
    BUTTON_COUNT
} button_t;

typedef struct window_t window_t;

typedef void (*KEY_CALLBACK)(window_t* window, keycode_t key, int pressed);
typedef void (*BUTTON_CALLBACK)(window_t* window, button_t button, int action);
typedef void (*SCROLL_CALLBACK)(window_t* window, double offset);
typedef void (*CURSOR_CALLBACK)(window_t* window, double xpos, double ypos);
typedef void (*CHAR_CALLBACK)(window_t* window, unsigned int codepoint);
typedef void (*FRAME_BUFFER_SIZE_CALLBACK)(window_t* window, int width, int height);

window_t *window_create(int width, int height, const char *title);
void window_terminate(window_t *handle);
int window_is_open(window_t* handle);
void window_poll_events(void);

void window_set_swap_interval(int interval);
int window_get_swap_interval(void);
void window_swap_bufers(window_t* handle);
double window_get_time(window_t* handle);

int is_key_pressed(keycode_t key);
int is_mouse_button_pressed(button_t button);

void window_size(window_t* handle, int *width, int *height);
void window_frame_buffer_size(window_t* handle, int *width, int *height);
void window_get_cursor_pos(window_t* handle, double *xpos, double *ypos);
void window_set_cursor_pos(window_t* handle, double xpos, double ypos);

void  window_set_user_pointer(window_t* handle, void *ptr);
void *window_get_user_pointer(window_t* handle);

void window_set_framebuffer_size_callback(window_t* handle, FRAME_BUFFER_SIZE_CALLBACK fun);
void window_set_cursor_pos_callback(window_t* window, CURSOR_CALLBACK fun);
void window_set_scroll_callback(window_t* window, SCROLL_CALLBACK fun);
void window_set_button_callback(window_t* window, BUTTON_CALLBACK fun);
void window_set_key_callback(window_t* window, KEY_CALLBACK fun);
void window_set_char_callback(window_t* window, CHAR_CALLBACK fun);

#endif

