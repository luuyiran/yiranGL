#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include <windows.h>

#include "window.h"

#pragma comment(lib, "opengl32.lib")

#define WNDCLASSNAME         L"YIRAN_1.0"
#define WINDOW_ENTRY         L"YIRAN"

#define OPENGL_MAJOR_VERSION 4
#define OPENGL_MINOR_VERSION 5

typedef struct win32_t {
    int         width;
    int         height;
    const char  *title;

    int         is_open;
    
    HWND        handle;
    HDC         dc;
    HGLRC       wgl;
    uint16_t    surrogate;

    uint64_t    frequency;
    uint64_t    time_offset;
    
    void        *user_ptr;

    struct {
        KEY_CALLBACK    key;
        BUTTON_CALLBACK button;
        SCROLL_CALLBACK scroll;
        CURSOR_CALLBACK cursor;
        CHAR_CALLBACK   character;
        FRAME_BUFFER_SIZE_CALLBACK fb_size;
    } callbacks;
} win32_t;

static LRESULT CALLBACK windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/*
 ***********************************************************************************
 * create window, setup OpenGl context
 ************************************************************************************
 */
static int register_window_class(void) {
    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = (WNDPROC)windowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName = 0;
    wc.lpszClassName = WNDCLASSNAME;
    if (!RegisterClassEx(&wc))  {
        fprintf(stderr, "Win32: Failed to register window class.\n");
        return YIRAN_FALSE;
    }
    return YIRAN_TRUE;
}

static uint64_t get_timer_value(void) {
    uint64_t value;
    QueryPerformanceCounter((LARGE_INTEGER*) &value);
    return value;
}

static int init_timer_win32(win32_t *win32) {
    assert(win32);
    if (!QueryPerformanceFrequency((LARGE_INTEGER*)&win32->frequency))
        return YIRAN_FALSE;
    win32->time_offset = get_timer_value();
    return YIRAN_TRUE;
}

static int init_win32(win32_t *win32) {
    assert(win32);
	if (!register_window_class())
        return YIRAN_FALSE;
	if (!init_timer_win32(win32))
        return YIRAN_FALSE;
    return YIRAN_TRUE;
}

static int create_native_window(win32_t *win32) {
    int      count = 0;
    WCHAR   *title = NULL;
    DWORD    style;
    RECT     window_rect;
    int      screen_width = GetSystemMetrics(SM_CXSCREEN);
    int      screen_height = GetSystemMetrics(SM_CYSCREEN);

    assert(win32);

    SetRect(&window_rect, (screen_width / 2) - (win32->width / 2), (screen_height / 2) - (win32->height / 2), (screen_width / 2) + (win32->width / 2), (screen_height / 2) + (win32->height / 2));

    count = MultiByteToWideChar(CP_UTF8, 0, win32->title, -1, NULL, 0);
    if (count) {
        title = calloc(count + 1, sizeof(WCHAR));
        MultiByteToWideChar(CP_UTF8, 0, win32->title, -1, title, count);
    }

    style = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    AdjustWindowRectEx(&window_rect, style, FALSE, 0);
    win32->handle = CreateWindowEx(0, WNDCLASSNAME, title, style, window_rect.left, window_rect.top, window_rect.right - window_rect.left, window_rect.bottom - window_rect.top, NULL, NULL, GetModuleHandle(NULL), GetCommandLineA());
    free(title);
    if (!win32->handle) {
        fprintf(stderr, " WIN32: Failed to CreateWindowEx.\n");
        return YIRAN_FALSE;
    }

	if (!SetProp(win32->handle, WINDOW_ENTRY, win32)) {
        fprintf(stderr, " WIN32: Failed to SetProp.\n");
        return YIRAN_FALSE;
    }
    return YIRAN_TRUE;
}

#define WGL_CONTEXT_MAJOR_VERSION_ARB               0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB               0x2092
#define WGL_CONTEXT_FLAGS_ARB                       0x2094
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB            0x00000001
#define WGL_CONTEXT_DEBUG_BIT_ARB                   0x00000001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB      0x00000002
#define WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB           0x00000004
#define WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB 0x8256
#define WGL_NO_RESET_NOTIFICATION_ARB               0x8261
#define WGL_CONTEXT_PROFILE_MASK_ARB                0x9126

typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC) (HDC, HGLRC, const int*);
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = NULL;
typedef const char* (WINAPI* PFNWGLGETEXTENSIONSSTRINGEXTPROC) (void);
PFNWGLGETEXTENSIONSSTRINGEXTPROC wglGetExtensionsStringEXT = NULL;
typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC) (int);
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;
typedef int (WINAPI* PFNWGLGETSWAPINTERVALEXTPROC) (void);
PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT = NULL;

static int vsynch = 0;
static int swap_control_supported = 0;

#define set_attrib(a, v) do { \
    assert(((size_t) index + 1) < sizeof(attribs) / sizeof(attribs[0])); \
    attribs[index++] = a; \
    attribs[index++] = v; \
} while(0)

static int create_wgl_contex(win32_t *win32) {
    int attribs[40] = {0};
    int index = 0, mask = 0, flags = 0;
    HGLRC tmpwgl;
    int pixelFormat;
    PIXELFORMATDESCRIPTOR pfd;
    
    assert(win32);

    mask  |= WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
    flags |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
    flags |= WGL_CONTEXT_DEBUG_BIT_ARB;
    flags |= WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB;

    set_attrib(WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB, WGL_NO_RESET_NOTIFICATION_ARB);
    set_attrib(WGL_CONTEXT_MAJOR_VERSION_ARB, OPENGL_MAJOR_VERSION);
    set_attrib(WGL_CONTEXT_MINOR_VERSION_ARB, OPENGL_MINOR_VERSION);
    set_attrib(WGL_CONTEXT_FLAGS_ARB, flags);
    set_attrib(WGL_CONTEXT_PROFILE_MASK_ARB, mask);
    set_attrib(0, 0);

    win32->dc = GetDC(win32->handle);
    if (!win32->dc) {
        fprintf(stderr, "WGL: Failed to retrieve DC for window.\n");
        return YIRAN_FALSE;
    }

    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 32;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;
    pixelFormat = ChoosePixelFormat(win32->dc, &pfd);
    SetPixelFormat(win32->dc, pixelFormat, &pfd);

    tmpwgl = wglCreateContext(win32->dc);
    if (!tmpwgl) {
        fprintf(stderr, "WGL: Failed to create tmp OpenGL context");
        return YIRAN_FALSE;
    }
    wglMakeCurrent(win32->dc, tmpwgl);

    wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglGetProcAddress("wglGetExtensionsStringEXT");
    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");
    wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    if (!wglCreateContextAttribsARB) {
        fprintf(stderr, "WGL: wglCreateContextAttribsARB() not found...\n");
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(tmpwgl);
        return YIRAN_FALSE;
    }
    win32->wgl = wglCreateContextAttribsARB(win32->dc, 0, attribs);
    if (!win32->wgl) {
        fprintf(stderr, "WGL: Failed to CreateContextAttribsARB for window.\n");
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(tmpwgl);
        return YIRAN_FALSE;
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(tmpwgl);
    wglMakeCurrent(win32->dc, win32->wgl);

    swap_control_supported = strstr(wglGetExtensionsStringEXT(), "WGL_EXT_swap_control") != NULL;
    window_set_swap_interval(1);

    return YIRAN_TRUE;
}


void window_set_swap_interval(int interval) {
    if (swap_control_supported) {
        wglSwapIntervalEXT(interval);
        vsynch = wglGetSwapIntervalEXT();
    } else 
        vsynch = 0;
}

int window_get_swap_interval(void) {
    return vsynch;
}

window_t* window_create(int width, int height, const char* title) {
    win32_t* win = NULL;

    assert(title);
    assert(width >= 0);
    assert(height >= 0);

    win = (win32_t*)calloc(1, sizeof(win32_t));
    if (NULL == win) {
        fprintf(stderr, "WIN32: Could not malloc space for WIN32!\n");
        return NULL;
    }

    win->width = width;
    win->height= height;
    win->title = title;

	if (!init_win32(win)) {
        fprintf(stderr, "WIN32: init_win32 error!\n");
        window_terminate((window_t *)win);
        return NULL;
    }
	if (!create_native_window(win)) {
        fprintf(stderr, " WIN32: create_native_window error!\n");
        window_terminate((window_t *)win);
        return NULL;
    }
	if (!create_wgl_contex(win)) {
        fprintf(stderr, " WIN32: create_wgl_contex error!\n");
        window_terminate((window_t *)win);
        return NULL;
    }
    ShowWindow(win->handle, SW_SHOW);
    UpdateWindow(win->handle);
    win->is_open = YIRAN_TRUE;
    return (window_t*)win;
}

void window_terminate(window_t *handle) {
    win32_t* win = (win32_t*)handle;
    if (NULL == win)   
        return;
    memset(&win->callbacks, 0, sizeof(win->callbacks));
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(win->wgl);
    if (win->handle) {
        RemovePropW(win->handle, WINDOW_ENTRY);
        DestroyWindow(win->handle);
    }
    UnregisterClassW(WNDCLASSNAME, GetModuleHandleW(NULL));
    free(win);
}


/*
 ***********************************************************************************
 * Process Input events.
 ************************************************************************************
 */
static void input_key(win32_t* handle, WPARAM virtual_key, char action) {
    keycode_t key;
    switch (virtual_key) {
    default:       return;
    case 'A':      key = KEY_A;     break;
    case 'B':      key = KEY_B;     break;
    case 'C':      key = KEY_C;     break;
    case 'D':      key = KEY_D;     break;
    case 'E':      key = KEY_E;     break;
    case 'S':      key = KEY_S;     break;
    case 'R':      key = KEY_R;     break;
    case 'V':      key = KEY_V;     break;
    case 'W':      key = KEY_W;     break;
    case 'X':      key = KEY_X;     break;
    case 'Z':      key = KEY_Z;     break;

    case VK_SPACE: key = KEY_SPACE; break;
    case VK_LEFT:  key = KEY_LEFT;  break;
    case VK_RIGHT: key = KEY_RIGHT; break;
    case VK_DOWN:  key = KEY_DOWN;  break;
    case VK_UP:    key = KEY_UP;    break;

    case VK_DELETE: key = KEY_DELETE;   break;
    case VK_RETURN: key = KEY_ENTER;    break;
    case VK_TAB:    key = KEY_TAB;      break;
    case VK_BACK:   key = KEY_BACKSPACE;break;

    case VK_END:     key = KEY_END;             break;
    case VK_HOME:    key = KEY_HOME;            break;
    case VK_NEXT:    key = KEY_PAGE_DOWN;       break;
    case VK_PRIOR:   key = KEY_PAGE_UP;         break;
    case VK_LSHIFT:  key = KEY_LEFT_SHIFT;      break;
    case VK_RSHIFT:  key = KEY_RIGHT_SHIFT;     break;
    case VK_LCONTROL:key = KEY_LEFT_CONTROL;    break;
    case VK_RCONTROL:key = KEY_RIGHT_CONTROL;   break;
    }

    if (handle->callbacks.key)
        handle->callbacks.key((window_t *)handle, key, action);

}

static void input_mouse_click(win32_t* handle, button_t button, char action) {
    if (handle->callbacks.button) {
        handle->callbacks.button((window_t*)handle, button, action);
    }
}

static void input_scroll(win32_t* handle, double yoffset) {
    if (handle->callbacks.scroll)
        handle->callbacks.scroll((window_t*)handle, yoffset);
}

static void input_cursor_pos(win32_t* handle, double xpos, double ypos) {
    if (handle->callbacks.cursor)
        handle->callbacks.cursor((window_t*)handle, xpos, ypos);
}

static void input_fb_size(win32_t* handle, int width, int height) {
    if (handle->callbacks.fb_size)
        handle->callbacks.fb_size((window_t*)handle, width, height);
}

static void input_char(win32_t *window, unsigned int codepoint) {
    if (codepoint < 32 || (codepoint > 126 && codepoint < 160))
        return;
    if (window->callbacks.character)
        window->callbacks.character((window_t*)window, codepoint);
}

static void set_tracking(win32_t* win32, int track) {
    TRACKMOUSEEVENT mouseEvent;
    mouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
    mouseEvent.dwFlags = track ? TME_LEAVE : TME_CANCEL;
    mouseEvent.hwndTrack = win32->handle;
    mouseEvent.dwHoverTime = HOVER_DEFAULT;
    TrackMouseEvent(&mouseEvent);
}

static LRESULT CALLBACK windowProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
    RECT        area;
    uint32_t    codepoint = 0;
    int32_t     x, y, width, height;
    win32_t    *window = (win32_t*)GetProp(hwnd, WINDOW_ENTRY);

    if (window) {
        switch (iMsg) {
        case WM_CLOSE: 
            window->is_open = YIRAN_FALSE; 
            return 0;
        case WM_KEYDOWN: 
            input_key(window, wParam, YIRAN_PRESS);
            return 0;
        case WM_KEYUP: 
            input_key(window, wParam, YIRAN_RELEASE); 
            return 0;
        case WM_LBUTTONDOWN: 
            input_mouse_click(window, BUTTON_LEFT, YIRAN_PRESS);
            return 0;
        case WM_RBUTTONDOWN: 
            input_mouse_click(window, BUTTON_RIGHT, YIRAN_PRESS); 
            return 0;
        case WM_LBUTTONUP: 
            input_mouse_click(window, BUTTON_LEFT, YIRAN_RELEASE); 
            return 0;
        case WM_RBUTTONUP: 
            input_mouse_click(window, BUTTON_RIGHT, YIRAN_RELEASE); 
            return 0;
        case WM_MBUTTONDOWN:
            input_mouse_click(window, BUTTON_MIDDLE, YIRAN_PRESS); 
            return 0;
        case WM_MBUTTONUP:
            input_mouse_click(window, BUTTON_MIDDLE, YIRAN_RELEASE); 
            return 0;
        case WM_MOUSEWHEEL: 
            input_scroll(window, (SHORT)HIWORD(wParam) / (double)WHEEL_DELTA);
            return 0;
        case WM_SIZE: {
            GetClientRect(window->handle, &area);
            width = area.right - area.left;
            height= area.bottom - area.top;
            input_fb_size(window, width, height);
            return 0;
        }
        case WM_MOUSEMOVE: {
            x = (signed short)(LOWORD(lParam));
            y = (signed short)(HIWORD(lParam));
            GetClientRect(window->handle, &area);

            /* Capture the mouse in case the user wants to drag it outside */
            if ((wParam & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON | MK_XBUTTON1 | MK_XBUTTON2)) == 0) {
                /* Only release the capture if we really have it */
                if (GetCapture() == window->handle)
                    ReleaseCapture();
            }
            else if (GetCapture() != window->handle) {
                /* Set the capture to continue receiving mouse events */
                SetCapture(window->handle);
            }
            /* If the cursor is outside the client area... */
            if ((x < area.left) || (x > area.right) || (y < area.top) || (y > area.bottom))
                set_tracking(window, YIRAN_FALSE);  /* No longer care for the mouse leaving the window */
            else
                set_tracking(window, YIRAN_TRUE);

            input_cursor_pos(window, x, y);
            return 0;
        }        
        case WM_CHAR: {
            if (wParam >= 0xd800 && wParam <= 0xdbff)
                window->surrogate = (WCHAR) wParam;
            else {
                codepoint = 0;
                if (wParam >= 0xdc00 && wParam <= 0xdfff) {
                    if (window->surrogate) {
                        codepoint += (window->surrogate - 0xd800) << 10;
                        codepoint += (WCHAR) wParam - 0xdc00;
                        codepoint += 0x10000;
                    }
                } else
                    codepoint = (WCHAR) wParam;

                window->surrogate = 0;
                input_char(window, codepoint);
            }
            return 0;
        }  
        default:
            break;
        }
    }
    return DefWindowProc(hwnd, iMsg, wParam, lParam);
}


/*
 ***********************************************************************************
 * API imp
 ************************************************************************************
 */
int window_is_open(window_t* handle) {
    win32_t* win = (win32_t*)handle;
    assert(win);
    return win->is_open;
}

void window_poll_events(void) {
    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
    /* if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) { */
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void window_swap_bufers(window_t* handle) {
    win32_t* win = (win32_t*)handle;
    assert(win);
    SwapBuffers(win->dc);
}

void window_size(window_t* handle, int *width, int *height) {
    win32_t* win = (win32_t*)handle;
    assert(win);

    RECT area;
    GetClientRect(win->handle, &area);

    if (width)
        *width = area.right;
    if (height)
        *height = area.bottom;
}

void window_frame_buffer_size(window_t* handle, int *width, int *height) {
    window_size(handle, width, height);
}

void window_set_user_pointer(window_t* handle, void *ptr) {
    win32_t* win = (win32_t*)handle;
    assert(win);
    win->user_ptr = ptr;
}

void *window_get_user_pointer(window_t* handle) {
    win32_t* win = (win32_t*)handle;
    assert(win);
    return win->user_ptr;
}

void window_get_cursor_pos(window_t* handle, double *xpos, double *ypos) {
    win32_t* win = (win32_t*)handle;
    assert(win);
    POINT pos;

    if (GetCursorPos(&pos)){
        ScreenToClient(win->handle, &pos);
        if (xpos)
            *xpos = pos.x;
        if (ypos)
            *ypos = pos.y;
    }
}

void window_set_cursor_pos(window_t* handle, double xpos, double ypos) {
    (void)handle;
    SetCursorPos((int)xpos, (int)ypos);
}

static uint64_t get_timer_frequency(win32_t* win32) {
    return win32->frequency;
}

double window_get_time(window_t* handle) {
    win32_t* win32 = (win32_t*)handle;
    if (NULL == win32)  return 0.0;
    return (double) (get_timer_value() - win32->time_offset) /
           (double) get_timer_frequency(win32);
}

void window_set_framebuffer_size_callback(window_t* handle, FRAME_BUFFER_SIZE_CALLBACK fun) {
    win32_t* win = (win32_t*)handle;
    assert(win);
    win->callbacks.fb_size = fun;
}

void window_set_cursor_pos_callback(window_t* handle, CURSOR_CALLBACK fun) {
    win32_t* win = (win32_t*)handle;
    assert(win);
    win->callbacks.cursor = fun;
}

void window_set_scroll_callback(window_t* handle, SCROLL_CALLBACK fun) {
    win32_t* win = (win32_t*)handle;
    assert(win);
    win->callbacks.scroll = fun;
}

void window_set_button_callback(window_t* handle, BUTTON_CALLBACK fun) {
    win32_t* win = (win32_t*)handle;
    assert(win);
    win->callbacks.button = fun;
}

void window_set_key_callback(window_t* handle, KEY_CALLBACK fun) {
    win32_t* win = (win32_t*)handle;
    assert(win);
    win->callbacks.key = fun;
}

void window_set_char_callback(window_t* handle, CHAR_CALLBACK fun) {
    win32_t* win = (win32_t*)handle;
    assert(win);
    win->callbacks.character = fun;
}

int is_key_pressed(keycode_t key) {
    int vkey = 0;
    switch (key) {
        default:         return YIRAN_FALSE;
        case KEY_A:      vkey = 'A';     break;
        case KEY_B:      vkey = 'B';     break;
        case KEY_C:      vkey = 'C';     break;
        case KEY_D:      vkey = 'D';     break;
        case KEY_E:      vkey = 'E';     break;
        case KEY_S:      vkey = 'S';     break;
        case KEY_R:      vkey = 'R';     break;
        case KEY_V:      vkey = 'V';     break;
        case KEY_W:      vkey = 'W';     break;
        case KEY_X:      vkey = 'X';     break;
        case KEY_Z:      vkey = 'Z';     break;

        case KEY_LEFT:   vkey = VK_LEFT; break;
        case KEY_RIGHT:  vkey = VK_RIGHT;break;
        case KEY_UP:     vkey = VK_UP;   break;
        case KEY_DOWN:   vkey = VK_DOWN; break;
        case KEY_SPACE:  vkey = VK_SPACE;break;

        case KEY_DELETE:        vkey = VK_DELETE;   break;
        case KEY_ENTER:         vkey = VK_RETURN;   break;
        case KEY_TAB:           vkey = VK_TAB;      break;
        case KEY_BACKSPACE:     vkey = VK_BACK;     break;
        case KEY_END:           vkey = VK_END;      break;
        case KEY_HOME:          vkey = VK_HOME;     break;
        case KEY_PAGE_DOWN:     vkey = VK_NEXT;     break;
        case KEY_PAGE_UP:       vkey = VK_PRIOR;    break;
        case KEY_LEFT_SHIFT:    vkey = VK_LSHIFT;   break;
        case KEY_RIGHT_SHIFT:   vkey = VK_RSHIFT;   break;
        case KEY_LEFT_CONTROL:  vkey = VK_LCONTROL; break;
        case KEY_RIGHT_CONTROL: vkey = VK_RCONTROL; break;
    }

    return (GetAsyncKeyState(vkey) & 0x8000) != 0;
}

int is_mouse_button_pressed(button_t button) {
    int vkey = 0;
    switch (button) {
        default:              return YIRAN_FALSE;
        case BUTTON_LEFT:     vkey = GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON; break;
        case BUTTON_RIGHT:    vkey = GetSystemMetrics(SM_SWAPBUTTON) ? VK_LBUTTON : VK_RBUTTON; break;
        case BUTTON_MIDDLE:   vkey = VK_MBUTTON;  break;
    
    }
    return (GetAsyncKeyState(vkey) & 0x8000) != 0;
}
