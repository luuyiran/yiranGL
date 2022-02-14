#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>

#include "window.h"

#define OPENGL_MAJOR_VERSION 4
#define OPENGL_MINOR_VERSION 5

typedef struct x11_t {
    int          width;
    int          height;
    const char  *title;
    char         is_open;

    int          screen;
    XIM          im;
    XIC          ic;
    Window       handle;
    Display     *display;
    GLXContext   glx;
    
    void        *user_ptr;

    uint64_t    frequency;
    uint64_t    time_offset;

    struct {
        KEY_CALLBACK    key;
        BUTTON_CALLBACK button;
        SCROLL_CALLBACK scroll;
        CURSOR_CALLBACK cursor;
        CHAR_CALLBACK   character;
        FRAME_BUFFER_SIZE_CALLBACK fb_size;
    } callbacks;
} x11_t;


static x11_t *x11_ptr = NULL;

/*
 ***********************************************************************************
 * create window, setup OpenGl context
 ************************************************************************************
 */
static uint64_t get_timer_value(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t) tv.tv_sec * (uint64_t) 1000000 + (uint64_t) tv.tv_usec;
}

static int init_x11(x11_t *x11) {
    assert(x11);

    x11->display = XOpenDisplay(NULL);
    if (!x11->display) {
        fprintf(stderr, "X11: Failed to Open Display!\n");
        return YIRAN_FALSE;
    }

    x11->screen = DefaultScreen(x11->display);

    x11->im = XOpenIM(x11->display, 0, NULL, NULL);
    if (!x11->im) {
        fprintf(stderr, "X11: XOpenIM Failed!\n");
        return YIRAN_FALSE;
    }

    x11->frequency = 1000000;
    x11->time_offset = get_timer_value();
    return YIRAN_TRUE;
}

static int init_glx(x11_t *x11, XVisualInfo *xvi, GLXFBConfig *fbcfg) {
    XVisualInfo *vi = NULL;
    GLXFBConfig *fbc = NULL;
    int glx_major, glx_minor, fbcount;

    /* Get a matching FB config */
    int visual_attribs[] = {
        GLX_X_RENDERABLE, True, GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, False,
        /* GLX_SAMPLE_BUFFERS  , 1, */
        /* GLX_SAMPLES         , 4, */
        None};

    assert(x11);
    assert(xvi);
    assert(fbcfg);
    /* FBConfigs were added in GLX version 1.3. */
    if (!glXQueryVersion(x11->display, &glx_major, &glx_minor) ||
        ((glx_major == 1) && (glx_minor < 3)) || (glx_major < 1)) {
        fprintf(stderr, "GLX: Invalid GLX version: %d.%d\n", glx_major, glx_minor);
        return YIRAN_FALSE;
    }

    fbc = glXChooseFBConfig(x11->display, x11->screen, visual_attribs, &fbcount);
    if (NULL == fbc || fbcount < 1) {
        fprintf(stderr, "GLX: Failed to retrieve a framebuffer config\n");
        return YIRAN_FALSE;
    }

    /* choose first one simply  */
    vi = glXGetVisualFromFBConfig(x11->display, fbc[0]);
    if (NULL == vi) {
        fprintf(stderr, "GLX: Failed to Get Visual From FBConfig.\n");
        XFree(fbc);
        return YIRAN_FALSE;
    }
    *xvi = *vi;
    *fbcfg = fbc[0];
    XFree(vi);
    /* Be sure to free the FBConfig list allocated by glXChooseFBConfig()  */
    XFree(fbc);
    return YIRAN_TRUE;
}

static int create_native_window(x11_t *x11, XVisualInfo *vi) {
    XSetWindowAttributes swa;
    assert(x11);
    assert(vi);

    memset(&swa, 0, sizeof(XSetWindowAttributes));
    swa.colormap = XCreateColormap(x11->display, RootWindow(x11->display, vi->screen), vi->visual, AllocNone);
    swa.background_pixmap = None;
    swa.border_pixel = 0;
    swa.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask |
                     PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
                     ExposureMask | FocusChangeMask | VisibilityChangeMask |
                     EnterWindowMask | LeaveWindowMask | PropertyChangeMask;

    x11->handle = XCreateWindow(x11->display, RootWindow(x11->display, vi->screen), 0, 0,
                               x11->width, x11->height, 0, vi->depth, InputOutput, vi->visual,
                               CWBorderPixel | CWColormap | CWEventMask, &swa);
    if (!x11->handle) {
        fprintf(stderr, "X11: Failed to create window.\n");
        return YIRAN_FALSE;
    }

    if (x11->im) {
        x11->ic = XCreateIC(x11->im, XNInputStyle,
                                     XIMPreeditNothing | XIMStatusNothing,
                                     XNClientWindow,
                                     x11->handle,
                                     XNFocusWindow,
                                     x11->handle,
                                     NULL);
    }

    if (x11->ic) {
        unsigned long filter = 0;
        if (XGetICValues(x11->ic, XNFilterEvents, &filter, NULL) == NULL) {
            XSelectInput(x11->display, x11->handle, swa.event_mask | filter);
        }
    }

    XStoreName(x11->display, x11->handle, x11->title);
    XMapWindow(x11->display, x11->handle);
    XFlush(x11->display);
    return YIRAN_TRUE;
}

#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092

typedef GLXContext (*PFNGLX_CREATECONTEXTATTRIBSARB)(Display *, GLXFBConfig, GLXContext, Bool, const int *);
PFNGLX_CREATECONTEXTATTRIBSARB glXCreateContextAttribsARB = NULL;
typedef void (*PFNGLX_SWAPINTERVALEXTPROC)(Display*, XID, int);
PFNGLX_SWAPINTERVALEXTPROC SwapIntervalEXT = NULL;
typedef int (*PFNGLXS_WAPINTERVALMESAPROC)(int);
PFNGLXS_WAPINTERVALMESAPROC SwapIntervalMESA = NULL;

static int vsynch = 0;
static int EXT_swap_control = 0;
static int MESA_swap_control = 0;

#define set_attrib(a, v) do { \
    assert(((size_t) index + 1) < sizeof(attribs) / sizeof(attribs[0])); \
    attribs[index++] = a; \
    attribs[index++] = v; \
} while(0)



static int create_glx_contex(x11_t *x11, GLXFBConfig *fbcfg) {
    int attribs[40] = {0};
    int index = 0, mask = 0, flags = 0;
    
    assert(x11);
    assert(fbcfg);

    SwapIntervalEXT = (PFNGLX_SWAPINTERVALEXTPROC)glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalEXT");
    SwapIntervalMESA = (PFNGLXS_WAPINTERVALMESAPROC)glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalMESA");
    glXCreateContextAttribsARB = (PFNGLX_CREATECONTEXTATTRIBSARB)glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");
    if (!glXCreateContextAttribsARB) {
        fprintf(stderr, "GLX: glXCreateContextAttribsARB() not found...\n");
        return YIRAN_FALSE;
    }

    EXT_swap_control = SwapIntervalEXT != NULL;
    MESA_swap_control = SwapIntervalMESA != NULL;

    mask  |= GLX_CONTEXT_CORE_PROFILE_BIT_ARB;
    flags |= GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
    flags |= GLX_CONTEXT_ROBUST_ACCESS_BIT_ARB;
    flags |= GLX_CONTEXT_DEBUG_BIT_ARB;

    set_attrib(GLX_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB, GLX_NO_RESET_NOTIFICATION_ARB);
    set_attrib(GLX_CONTEXT_MAJOR_VERSION_ARB, OPENGL_MAJOR_VERSION);
    set_attrib(GLX_CONTEXT_MINOR_VERSION_ARB, OPENGL_MINOR_VERSION);
    set_attrib(GLX_CONTEXT_FLAGS_ARB, flags);
    set_attrib(GLX_CONTEXT_PROFILE_MASK_ARB, mask);
    set_attrib(None, None);

    x11->glx = glXCreateContextAttribsARB(x11->display, *fbcfg, 0, True, attribs);
    if (!x11->glx) {
        fprintf(stderr, "GLX: Failed to Create New Context.\n");
        return YIRAN_FALSE;
    }

    XSync(x11->display, False);

    if (!glXMakeCurrent(x11->display, x11->handle, x11->glx)) {
        fprintf(stderr, "GLX: Failed to Make Current Context.\n");
        return YIRAN_FALSE;
    }

    window_set_swap_interval(1);
    return YIRAN_TRUE;
}

window_t *window_create(int width, int height, const char *title) {
    x11_t* win = NULL;
    XVisualInfo vi;
    GLXFBConfig fbcfg;

    assert(title);
    assert(width >= 0);
    assert(height >= 0);

    win = (x11_t*)calloc(1, sizeof(x11_t));
    if (NULL == win) {
        fprintf(stderr, "X11: Could not malloc space for X11!\n");
        return NULL;
    }

    memset(&vi, 0, sizeof(XVisualInfo));
    memset(&fbcfg, 0, sizeof(GLXFBConfig));

    win->width = width;
    win->height= height;
    win->title = title;
    x11_ptr = win;

	if (!init_x11(win)) {
        fprintf(stderr, " X11: init X11 error!\n");
        window_terminate((window_t *)win);
        return NULL;
    }

    if (!init_glx(win, &vi, &fbcfg)) {
        fprintf(stderr, " GLX: init GLX error!\n");
        window_terminate((window_t *)win);
        return NULL;
    }

    if (!create_native_window(win, &vi)) {
        fprintf(stderr, " X11: create_native_window error!\n");
        window_terminate((window_t *)win);
        return NULL;
    }
    
    if (!create_glx_contex(win, &fbcfg)) {
        fprintf(stderr, " GLX: create_glx_contex error!\n");
        window_terminate((window_t *)win);
        return NULL;
    }
    win->is_open = YIRAN_TRUE;
    return (window_t*)win;
}

void window_terminate(window_t *handle) {
    x11_t *x11 = (x11_t *)handle;
    if (NULL == x11)
        return;

    memset(&x11->callbacks, 0, sizeof(x11->callbacks));
    glXMakeCurrent(x11->display, None, NULL);

    if (x11->ic)
        XDestroyIC(x11->ic);

    if (x11->handle) {
        XUnmapWindow(x11->display, x11->handle);
        XDestroyWindow(x11->display, x11->handle);
    }

    if (x11->glx)
        glXDestroyContext(x11->display, x11->glx);

    if (x11->im) 
        XCloseIM(x11->im);

    if (x11->display)
        XCloseDisplay(x11->display);

    free(x11);
}

/*
 ***********************************************************************************
 * Process Input events.
 ************************************************************************************
 */
static void input_key(x11_t *handle, int virtual_key, int action) {
    KeySym *keysyms;
    KeySym keysym;
    keycode_t key;
    int dummy;

    keysyms = XGetKeyboardMapping(handle->display, virtual_key, 1, &dummy);
    keysym = keysyms[0];
    XFree(keysyms);

    switch (keysym) {
        default:       return;
        case XK_a:     key = KEY_A;     break;
        case XK_b:     key = KEY_B;     break;
        case XK_c:     key = KEY_C;     break;
        case XK_d:     key = KEY_D;     break;
        case XK_e:     key = KEY_E;     break;
        case XK_s:     key = KEY_S;     break;
        case XK_r:     key = KEY_R;     break;
        case XK_v:     key = KEY_V;     break;
        case XK_w:     key = KEY_W;     break;
        case XK_x:     key = KEY_X;     break;
        case XK_z:     key = KEY_Z;     break;

        case XK_Left:  key = KEY_LEFT;  break;
        case XK_Right: key = KEY_RIGHT; break;
        case XK_Up:    key = KEY_UP;    break;
        case XK_Down:  key = KEY_DOWN;  break;
        case XK_space: key = KEY_SPACE; break;

        case XK_Delete:     key = KEY_DELETE;       break;
        case XK_Return:     key = KEY_ENTER;        break;
        case XK_Tab:        key = KEY_TAB;          break;
        case XK_BackSpace:  key = KEY_BACKSPACE;    break;
        case XK_End:        key = KEY_END;          break;
        case XK_Home:       key = KEY_HOME;         break;
        case XK_Next:       key = KEY_PAGE_DOWN;    break;
        case XK_Prior:      key = KEY_PAGE_UP;      break;
        case XK_Shift_L:    key = KEY_LEFT_SHIFT;   break;
        case XK_Shift_R:    key = KEY_RIGHT_SHIFT;  break;
        case XK_Control_L:  key = KEY_LEFT_CONTROL; break;
        case XK_Control_R:  key = KEY_RIGHT_CONTROL;break;
    }

    if (handle->callbacks.key)
        handle->callbacks.key((window_t*)handle, key, action);
}

static void input_mouse_click(x11_t *handle, button_t button, int action) {
    if (handle->callbacks.button)
        handle->callbacks.button((window_t*)handle, button, action);
}

static void input_scroll(x11_t* handle, double yoffset) {
    if (handle->callbacks.scroll)
        handle->callbacks.scroll((window_t*)handle, yoffset);
}

static void input_cursor_pos(x11_t* handle, double xpos, double ypos) {
    if (handle->callbacks.cursor)
        handle->callbacks.cursor((window_t*)handle, xpos, ypos);
}

static void input_fb_size(x11_t* handle, int width, int height) {
    if (handle->callbacks.fb_size)
        handle->callbacks.fb_size((window_t*)handle, width, height);
}

static void input_char(x11_t *window, unsigned int codepoint) {
    if (codepoint < 32 || (codepoint > 126 && codepoint < 160))
        return;
    if (window->callbacks.character)
        window->callbacks.character((window_t*)window, codepoint);
}

static void process_events(XEvent *event) {
    int i, count;
    uint32_t btn;
    int width, height;
    Status status;
    wchar_t buffer[128] = {0};
    wchar_t* chars = buffer;

    if (NULL == x11_ptr)    return;

    switch (event->type) {
    case KeyPress: {
        input_key(x11_ptr, event->xkey.keycode, YIRAN_PRESS);
        if (!XFilterEvent(event, None)) {
            count = XwcLookupString(x11_ptr->ic, &event->xkey, buffer, sizeof(buffer) / sizeof(wchar_t),  NULL, &status);
            if (status == XBufferOverflow) {
                chars = calloc(count, sizeof(wchar_t));
                count = XwcLookupString(x11_ptr->ic, &event->xkey, chars, count, NULL, &status);
            }
            if (status == XLookupChars || status == XLookupBoth) {
                for (i = 0;  i < count;  i++)
                    input_char(x11_ptr, chars[i]);
            }
            if (chars != buffer) free(chars);
        }
        break;
    }
    case KeyRelease:
        input_key(x11_ptr, event->xkey.keycode, YIRAN_RELEASE);
        break;
    case ButtonPress: {
        btn = event->xbutton.button;
        if (Button1 == btn)
            input_mouse_click(x11_ptr, BUTTON_LEFT, YIRAN_PRESS);
        else if (Button2 == btn)
            input_mouse_click(x11_ptr, BUTTON_MIDDLE, YIRAN_PRESS);
        else if (Button3 == btn)
            input_mouse_click(x11_ptr, BUTTON_RIGHT, YIRAN_PRESS);
        break;
    }
    case ButtonRelease: {
        btn = event->xbutton.button;
        if (Button1 == btn)
            input_mouse_click(x11_ptr, BUTTON_LEFT, YIRAN_RELEASE);
        else if (Button2 == btn)
            input_mouse_click(x11_ptr, BUTTON_MIDDLE, YIRAN_RELEASE);
        else if (Button3 == btn)
            input_mouse_click(x11_ptr, BUTTON_RIGHT, YIRAN_RELEASE);
        else if ((Button4 == btn) || (Button5 == btn))
            input_scroll(x11_ptr, (Button4 == btn) ? 1.0 : -1.0);
        break;
    }
    case MotionNotify:
        input_cursor_pos(x11_ptr, event->xmotion.x, event->xmotion.y);
        break;
    case ConfigureNotify:
        width = event->xconfigure.width;
        height= event->xconfigure.height;
        if (width != x11_ptr->width || height != x11_ptr->height) {
            x11_ptr->width = width;
            x11_ptr->height= height;
            input_fb_size(x11_ptr, width, height);
        }
        break;
    default:
        break;
    }
}

/*
 ***********************************************************************************
 * API imp
 ************************************************************************************
 */
int window_is_open(window_t* handle) {
    x11_t *x11 = (x11_t *)handle;
    assert(x11);
    return x11->is_open;
}

void window_poll_events(void) {
    XEvent event;
    assert(x11_ptr);
    XPending(x11_ptr->display);
    while (XQLength(x11_ptr->display)) {
        XNextEvent(x11_ptr->display, &event);
        process_events(&event);
    }
    XFlush(x11_ptr->display);
}

void window_set_swap_interval(int interval) {
    vsynch = interval;
    if (EXT_swap_control)
        SwapIntervalEXT(x11_ptr->display, glXGetCurrentDrawable(), interval);
    else if (MESA_swap_control)
        SwapIntervalMESA(interval);
    else 
        vsynch = 0;
}

int window_get_swap_interval(void) {
    return vsynch;
}

void window_swap_bufers(window_t* handle) {
    x11_t* win = (x11_t *)handle;
    assert(win);
    glXSwapBuffers(win->display, win->handle);
}

static uint64_t get_timer_frequency(x11_t* x11) {
    return x11->frequency;
}

double window_get_time(window_t* handle) {
    x11_t* x11 = (x11_t*)handle;
    assert(x11);
    return (double) (get_timer_value() - x11->time_offset) /
           (double) get_timer_frequency(x11);
}


int is_key_pressed(keycode_t key) {
    char keys[32] = {0};
    KeySym vkey = 0;
    KeyCode keycode = 0;

    switch (key) {
        default:         return YIRAN_FALSE;
        case KEY_A:      vkey = XK_a;     break;
        case KEY_B:      vkey = XK_b;     break;
        case KEY_C:      vkey = XK_c;     break;
        case KEY_D:      vkey = XK_d;     break;
        case KEY_E:      vkey = XK_e;     break;
        case KEY_S:      vkey = XK_s;     break;
        case KEY_R:      vkey = XK_r;     break;
        case KEY_V:      vkey = XK_v;     break;
        case KEY_W:      vkey = XK_w;     break;
        case KEY_X:      vkey = XK_x;     break;
        case KEY_Z:      vkey = XK_z;     break;

        case KEY_LEFT:   vkey = XK_Left;  break;
        case KEY_RIGHT:  vkey = XK_Right; break;
        case KEY_UP:     vkey = XK_Up;    break;
        case KEY_DOWN:   vkey = XK_Down;  break;
        case KEY_SPACE:  vkey = XK_space; break;

        case KEY_DELETE:        vkey = XK_Delete;   break;
        case KEY_ENTER:         vkey = XK_Return;   break;
        case KEY_TAB:           vkey = XK_Tab;      break;
        case KEY_BACKSPACE:     vkey = XK_BackSpace;break;
        case KEY_END:           vkey = XK_End;      break;
        case KEY_HOME:          vkey = XK_Home;     break;
        case KEY_PAGE_DOWN:     vkey = XK_Next;     break;
        case KEY_PAGE_UP:       vkey = XK_Prior;    break;
        case KEY_LEFT_SHIFT:    vkey = XK_Shift_L;  break;
        case KEY_RIGHT_SHIFT:   vkey = XK_Shift_R;  break;
        case KEY_LEFT_CONTROL:  vkey = XK_Control_L;break;
        case KEY_RIGHT_CONTROL: vkey = XK_Control_R;break;
    }

    keycode = XKeysymToKeycode(x11_ptr->display, vkey);
    if (keycode != 0){
        XQueryKeymap(x11_ptr->display, keys);
        return (keys[keycode / 8] & (1 << (keycode % 8))) != 0;
    }
    return YIRAN_FALSE;

}

int is_mouse_button_pressed(button_t button) {
    int wx, wy;
    int gx, gy;
    Window root, child;
    uint32_t buttons = 0;

    XQueryPointer(x11_ptr->display, DefaultRootWindow(x11_ptr->display), &root, &child, &gx, &gy, &wx, &wy, &buttons);

    switch (button){
        default:              return YIRAN_FALSE;
        case BUTTON_LEFT:     return !!(buttons & Button1Mask);
        case BUTTON_RIGHT:    return !!(buttons & Button3Mask);
        case BUTTON_MIDDLE:   return !!(buttons & Button2Mask);
    }

    return YIRAN_FALSE;
}

void window_size(window_t* handle, int *width, int *height) {
    x11_t* x11 = (x11_t*)handle;
    XWindowAttributes attribs;
    assert(x11);
    XGetWindowAttributes(x11->display, x11->handle, &attribs);
    if (width)
        *width = attribs.width;
    if (height)
        *height = attribs.height;
}

void window_frame_buffer_size(window_t* handle, int *width, int *height) {
    window_size(handle, width, height);
}

void window_get_cursor_pos(window_t* handle, double *xpos, double *ypos) {
    Window root, child;
    int rootX, rootY, childX, childY;
    unsigned int mask;
    x11_t* x11 = (x11_t*)handle;
    assert(x11);
    XQueryPointer(x11->display, x11->handle, &root, &child,
                  &rootX, &rootY, &childX, &childY, &mask);
    if (xpos)
        *xpos = childX;
    if (ypos)
        *ypos = childY;
}

void window_set_cursor_pos(window_t* handle, double xpos, double ypos) {
    x11_t* x11 = (x11_t*)handle;
    assert(x11);
    XWarpPointer(x11->display, None, DefaultRootWindow(x11->display), 0, 0, 0, 0, (int)xpos, (int)ypos);
    XFlush(x11->display);
}

void window_set_framebuffer_size_callback(window_t* handle, FRAME_BUFFER_SIZE_CALLBACK fun) {
    x11_t* win = (x11_t*)handle;
    assert(win);
    win->callbacks.fb_size = fun;
}

void window_set_cursor_pos_callback(window_t* handle, CURSOR_CALLBACK fun) {
    x11_t* win = (x11_t*)handle;
    assert(win);
    win->callbacks.cursor = fun;
}

void window_set_scroll_callback(window_t* handle, SCROLL_CALLBACK fun) {
    x11_t* win = (x11_t*)handle;
    assert(win);
    win->callbacks.scroll = fun;
}

void window_set_button_callback(window_t* handle, BUTTON_CALLBACK fun) {
    x11_t* win = (x11_t*)handle;
    assert(win);
    win->callbacks.button = fun;
}

void window_set_key_callback(window_t* handle, KEY_CALLBACK fun) {
    x11_t* win = (x11_t*)handle;
    assert(win);
    win->callbacks.key = fun;
}

void window_set_char_callback(window_t* handle, CHAR_CALLBACK fun) {
    x11_t* win = (x11_t*)handle;
    assert(win);
    win->callbacks.character = fun;
}

void window_set_user_pointer(window_t* handle, void *ptr) {
    x11_t* win = (x11_t*)handle;
    assert(win);
    win->user_ptr = ptr;
}

void *window_get_user_pointer(window_t* handle) {
    x11_t* win = (x11_t*)handle;
    assert(win);
    return win->user_ptr;
}
