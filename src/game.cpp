#include "game.h"
/* TODO: reiimplement game, to work linux native*/

#define internal static
#define local_persist static
#define global static

global bool global_running;
// TODO: *source_voice{0}

int main() {
    Display* display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Unable to open X display\n");
        return 1;
    }

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    int x = 100, y = 100;
    unsigned int width = 800, height = 600;
    unsigned int border_width = 0;

    XSetWindowAttributes attributes = {};
    attributes.background_pixel = 0xFFFFFF;//BG color
    attributes.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask; //input events to respond to

    unsigned long valuemask = CWBackPixel | CWEventMask;
    Window window = XCreateWindow(
        display,                    // The display pointer
        root,                       // Parent window (the desktop root)
        x, y,                       // Coordinates
        width, height,              // Sizes
        border_width,               // Border width
        CopyFromParent,             // Depth
        InputOutput,                // Window class
        CopyFromParent,             // Visual type
        valuemask,                  // Attribute mask
        &attributes                 // Pointer to the attribute structure
    );
  //TODO: add frequency_res, queryperffreq, define frequency

  if (window){
    Atom wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wmDeleteWindow, 1);

    XMapWindow(display, window);
    XFlush(display);

    global_running = true;
    /*TODO: 
      get window dimensions
      resize dib section
      load needed libraries, if needed

      TODO: setup sound structs and info
      TODO: setup memory, virtualalloc that mem, performance counting(last_counter, Query it, last cpu clock)
      only continue if audio, memory, ram all true 
      init game input

#if HANDMADE_DEV
      LPVOID base_address = (LPVOID)terabytes((uint64_t)2);
#else
      LPVOID base_address = 0;
#endif
 
    */

    XEvent event;

    while (global_running) {
        XNextEvent(display, &event);

        switch (event.type) {
            case Expose:
                // Window needs a redraw (triggered on creation/uncovering)
                break;

            case KeyPress: {
                // Check if the user pressed the Escape key to close the window
                // TODO: add KeyPress Handler function
                KeySym keysym = XLookupKeysym(&event.xkey, 0);
                if (keysym == XK_Escape) {
                    global_running = false;
                }
                break;
            }

            case ClientMessage:
                // Close the window if the user clicked the Window Manager close button
                if ((Atom)event.xclient.data.l[0] == wmDeleteWindow) {
                    global_running = false;
                }
                break;

            default:
                break;
        }
      //TODO: need to implement linux versions of this
        screen_dimensions_struct window_dimensions = get_window_dimensions(window);

        resize_window(&global_offscreenBuffer, windowDimensions.width,
                              windowDimensions.height);

        offscreen_buffer_struct offscreen_buffer{};
        offscreen_buffer.memory = global_offscreenBuffer.memory;
        offscreen_buffer.height = global_offscreenBuffer.height;
        offscreen_buffer.width = global_offscreenBuffer.width;
        offscreen_buffer.bytesPerPixel =
            global_offscreenBuffer.bytesPerPixel;
        offscreen_buffer.pitch = global_offscreenBuffer.pitch;

    }

    XDestroyWindow(display, window);
    XCloseDisplay(display);
  }
  return 0;
}
