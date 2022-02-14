/*
 ***********************************************************************************
 * OpenGL on Mac is deprecated, and the OpenGL version after 4.1 is not suported.
 * ... giving up debuging nsgl ...
 ************************************************************************************
 */

#define GL_SILENCE_DEPRECATION
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <sys/time.h>

#include <Carbon/Carbon.h>
#include <stdatomic.h>

#include <mach-o/dyld.h>
#include <mach/mach_time.h>
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

#error "OpenGL on Mac is deprecated, and the OpenGL version after 4.1 is not suported ... Giving up Debuging NSGL ..."

#if 0

#include "window.h"

typedef struct cocoa_t {
    int          width;
    int          height;
    const char  *title;
    char         is_open;

    id           handle;
    id           view;
    id           nsgl;
    id           framework;
    
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
} cocoa_t;


@interface content_view : NSView <NSTextInputClient> {
    cocoa_t* window;
    NSMutableAttributedString* markedText;
}

- (instancetype)init_with_window:(cocoa_t *)init_indow;
@end

@implementation content_view
- (instancetype)init_with_window:(cocoa_t *)init_indow {
    self = [super init];
    if (self != nil) {
        window = init_indow;
        markedText = [[NSMutableAttributedString alloc] init];
    }
    return self;
}
- (void)dealloc {
    [markedText release];
    [super dealloc];
}
@end


static uint64_t get_timer_value(void) {
    return mach_absolute_time();
}

static uint64_t get_timer_frequency(cocoa_t *ns) {
    return ns->frequency;
}

double window_get_time(window_t* handle) {
    cocoa_t* ns = (cocoa_t*)handle;
    assert(ns);
    return (double) (get_timer_value() - ns->time_offset) /
           (double) get_timer_frequency(ns);
}

int init_nsgl(cocoa_t *ns) {
    ns->framework = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.opengl"));
    if (ns->framework == NULL) {
        fprintf(stderr, "NSGL: Failed to locate OpenGL framework");
        return YIRAN_FALSE;
    }
    return YIRAN_TRUE;
}

static void init_timer(cocoa_t *ns) {
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);
    assert(ns);
    ns->frequency = (info.denom * 1e9) / info.numer;
    ns->time_offset = get_timer_value();
}

static void set_up_menubar(void) {
    [NSApplication sharedApplication]; /* Make sure NSApp exists */

    /* Set the main menu bar */
    NSMenu* mainMenu = [NSApp mainMenu];
    if (mainMenu != nil)
        return;
    mainMenu = [[[NSMenu alloc] initWithTitle:@""] autorelease];
    [NSApp setMainMenu:mainMenu];

    /* Application Menu (aka Apple Menu) */
    NSMenuItem* appleItem = [mainMenu addItemWithTitle:@"" action:nil keyEquivalent:@""];
    NSMenu* appleMenu = [[[NSMenu alloc] init] autorelease];
    [appleItem setSubmenu:appleMenu];

    /* File Menu */
    NSMenuItem* fileItem = [mainMenu addItemWithTitle:@"" action:nil keyEquivalent:@""];
    NSMenu* fileMenu = [[[NSMenu alloc] init] autorelease];
    [fileItem setSubmenu:fileMenu];

    /* Window menu */
    NSMenuItem* windowItem = [mainMenu addItemWithTitle:@"" action:nil keyEquivalent:@""];
    NSMenu* windowMenu = [[[NSMenu alloc] init] autorelease];
    [windowItem setSubmenu:windowMenu];
    [NSApp setWindowsMenu:windowMenu];
}


static void set_up_process() {
    if (NSApp == nil) {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        /* Create menus for the application (before finishing launching!) */
        set_up_menubar();
        /* Tell the application to stop bouncing in the Dock. */
        [NSApp finishLaunching];
    }
}


static int create_native_window(cocoa_t *ns) {
    NSRect rect;
    NSUInteger mask;

    content_view *view = [[content_view alloc] init_with_window:ns];
    set_up_process();
    
    rect = NSMakeRect(0, 0, ns->width, ns->height);
    mask = NSWindowStyleMaskTitled
           | NSWindowStyleMaskClosable
           | NSWindowStyleMaskMiniaturizable
           | NSWindowStyleMaskBorderless;
    ns->handle = [[NSWindow alloc] initWithContentRect:rect
                                         styleMask:mask
                                           backing:NSBackingStoreBuffered
                                             defer:NO];
    if (nil == ns->handle) {
        fprintf(stderr, "Cocoa: Failed to create window");
        return YIRAN_FALSE;
    }

    [ns->handle setTitle:[NSString stringWithUTF8String:ns->title]];
    [ns->handle setColorSpace:[NSColorSpace genericRGBColorSpace]];

    [ns->handle setContentView:view];
    [ns->handle makeFirstResponder:view];

    [ns->handle setDelegate:(NSObject <NSWindowDelegate> *)view];
    [ns->handle center];

    return YIRAN_TRUE;
}

#define add_attrib(a) do { \
    assert((size_t) index < sizeof(attribs) / sizeof(attribs[0])); \
    attribs[index++] = a; \
} while(0)
#define set_attrib(a, v) { add_attrib(a); add_attrib(v); }

static int create_nsgl_context(cocoa_t *ns) {
    int index = 0;
    NSOpenGLPixelFormatAttribute attribs[40];

    if (!init_nsgl(ns)) {
        fprintf(stderr, "NSGL: init error!\n");
        return YIRAN_FALSE;
    }

    add_attrib(NSOpenGLPFAAccelerated);
    add_attrib(NSOpenGLPFAClosestPolicy);
    set_attrib(NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion4_1Core);
    set_attrib(NSOpenGLPFAColorSize, 24);
    set_attrib(NSOpenGLPFAAlphaSize, 8);
    set_attrib(NSOpenGLPFADepthSize, 24);
    set_attrib(NSOpenGLPFAStencilSize, 8);
    add_attrib(NSOpenGLPFADoubleBuffer);
    set_attrib(NSOpenGLPFASampleBuffers, 1);
    /* set_attrib(NSOpenGLPFASamples, 4); */
    add_attrib(0);

    id pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
    if (pixelFormat == nil) {
        fprintf(stderr, "NSGL: Failed to find a suitable pixel format");
        return YIRAN_FALSE;
    }
    ns->nsgl = [[NSOpenGLContext alloc] initWithFormat:pixelFormat 
                                           shareContext:nil];
    if (ns->nsgl == nil) {
        fprintf(stderr, "NSGL: Failed to create OpenGL context");
        return YIRAN_FALSE;
    }

    [ns->view setWantsBestResolutionOpenGLSurface:1];

    [ns->nsgl setView:ns->view];

    return YIRAN_TRUE;
}

window_t *window_create(int width, int height, const char *title) {
    cocoa_t* ns = NULL;

    assert(title);
    assert(width >= 0);
    assert(height >= 0);

    ns = (cocoa_t*)calloc(1, sizeof(cocoa_t));
    if (NULL == ns) {
        fprintf(stderr, "Cocoa: Could not malloc space for cocoa_t!\n");
        return NULL;
    }
    ns->width = width;
    ns->height= height;
    ns->title = title;

    init_timer(ns);

    if (!create_native_window(ns)) {
        fprintf(stderr, " Cocoa: create_native_window error!\n");
        window_terminate((window_t *)ns);
        return NULL;
    }


    if (!create_nsgl_context(ns)) {
        fprintf(stderr, " NSGL: create_nsgl_context error!\n");
        window_terminate((window_t *)ns);
        return NULL;
    }

    ns->is_open = YIRAN_TRUE;
    return (window_t*)ns;
}
void window_terminate(window_t *handle) {
    // todo
}

void window_poll_events(void) {
    // todo
}

void window_swap_bufers(window_t* handle) {
    cocoa_t *ns = (cocoa_t *)handle;
    assert(ns);
    [ns->nsgl flushBuffer];
}

int is_key_pressed(keycode_t key) {
    //todo
    return 0;
}
int is_mouse_button_pressed(button_t button) {
    // todo
    return 0;
}


void window_size(window_t* handle, int *width, int *height) {}
void window_frame_buffer_size(window_t* handle, int *width, int *height) {}
void window_get_cursor_pos(window_t* handle, double *xpos, double *ypos) {}


int window_is_open(window_t* handle) {
    cocoa_t *ns = (cocoa_t *)handle;
    assert(ns);
    return ns->is_open;
}


void window_set_gl_version(int major, int minor) {
    (void)major;
    (void)minor;
}

void window_set_user_pointer(window_t* handle, void *ptr) {
    cocoa_t* win = (cocoa_t*)handle;
    assert(win);
    win->user_ptr = ptr;
}

void *window_get_user_pointer(window_t* handle) {
    cocoa_t* win = (cocoa_t*)handle;
    assert(win);
    return win->user_ptr;
}


void window_set_framebuffer_size_callback(window_t* handle, FRAME_BUFFER_SIZE_CALLBACK fun) {
    cocoa_t* win = (cocoa_t*)handle;
    assert(win);
    win->callbacks.fb_size = fun;
}

void window_set_cursor_pos_callback(window_t* handle, CURSOR_CALLBACK fun) {
    cocoa_t* win = (cocoa_t*)handle;
    assert(win);
    win->callbacks.cursor = fun;
}

void window_set_scroll_callback(window_t* handle, SCROLL_CALLBACK fun) {
    cocoa_t* win = (cocoa_t*)handle;
    assert(win);
    win->callbacks.scroll = fun;
}

void window_set_button_callback(window_t* handle, BUTTON_CALLBACK fun) {
    cocoa_t* win = (cocoa_t*)handle;
    assert(win);
    win->callbacks.button = fun;
}

void window_set_key_callback(window_t* handle, KEY_CALLBACK fun) {
    cocoa_t* win = (cocoa_t*)handle;
    assert(win);
    win->callbacks.key = fun;
}

void window_set_char_callback(window_t* handle, CHAR_CALLBACK fun) {
    cocoa_t* win = (cocoa_t*)handle;
    assert(win);
    win->callbacks.character = fun;
}

#endif 
