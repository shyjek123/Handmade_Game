#include "game.h"
#include <X11.h>
/* TODO: reiimplement game, to work linux native
 *  priorities: 
 *    1. blit frame buffer to the screen
 *    2. add color to the buffer
 *    3. animate that buffer
 *    4. add sound output (temporal sine wave sound)
 *    5. add keyboard input
 *    6. add xbox controller input
 *    7. have the screen move and sound buffer output sound according to keyboard and controller input
 */

struct linux_offscreen_buffer{
  XImage *image;
  void *memory;
  int width;
  int height;
  int bytesPerPixel;
  int pitch;
  size_t size;
};
global linux_offscreen_buffer global_offscreen_buffer;

struct screen_dimensions_struct{
  int width;
  int height;
};

internal screen_dimensions_struct linux_get_window_dimensions(Display display, Window window);
internal void linux_resize_image_section(linux_offscreen_buffer *buffer, int width, int height);
internal void linux_display_offscreen_buffer();

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
    global_running = true;

    Atom wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wmDeleteWindow, 1);

    XMapWindow(display, window);
    XFlush(display);

    screen_dimensions_struct dims = get_screen_dimensions(display, window);
    resize_screen(&global_offscreen_buffer, dims.width, dims.height);

    /*TODO: 
      1. setup sound structs and info
    */

#if HANDMADE_DEV
        void *base_address = (void *)terabytes((uint64_t)2);
#else
        void *base_address = 0;
#endif
/*
 *TODO:
  1. setup and alloc game memory
  2. check if everything alloced
  3. initialize input structs
 * */

    XEvent event;

    while (global_running) {
      /*TODO:
        * init game_controller/keyboard and zeroed input (read more about adding game controller/keyboard input)
        */
        XNextEvent(display, &event);

      //TODO: make sure all important msgs are being handled properly
        switch (event.type) {
            case Expose:
                // Window needs a redraw (triggered on creation/uncovering)
                break;

            case ConfigureNotify:
              //window resize
              int width = event.xconfigure.width;
              int height = event.xconfigure.height;

              //update frame buffer
              resize_offscreen_buffer(&buffer, width, height);

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

        screen_dimensions_struct screen_dims = get_screen_dimensions(display, window);

        resize_screen(&global_offscreen_buffer, screen_dims.width,
                              screen_dims.height);

        offscreen_buffer_struct offscreen_buffer{};
        offscreen_buffer.memory = global_offscreen_buffer.memory;
        offscreen_buffer.height = global_offscreen_buffer.height;
        offscreen_buffer.width = global_offscreen_buffer.width;
        offscreen_buffer.bytesPerPixel =
            global_offscreen_buffer.bytesPerPixel;
        offscreen_buffer.pitch = global_offscreen_buffer.pitch;

      display_offscreen_buffer();


      /*TODO: performance logging */

    }

    XDestroyWindow(display, window);
    XCloseDisplay(display);
  }else{
  //TODO: logging
  }
  return 0;
}


internal screen_dimensions_struct linux_get_window_dimensions(Display display, Window window){
  screen_dimensions_struct result;
  XWindowAttributes attrs;
  XGetWindowAttributes(display, window, &attrs);

  int width  = attrs.width;
  int height = attrs.height;
  result.width = width;
  result.height = height;

  return result;
}

internal void linux_resize_image_section(offscreen_buffer* buffer, int width, int height){
  if(buffer->memory){
    if (munmap(buffer->memory, buffer->size) == -1) {
        perror("munmap");
        return 1;
    }
  }

  if(buffer->image){
    XDestoryImage(buffer->image);
  }

  //TODO: add XCreateImage logic and assign it to buffer->image
  buffer->width = width;
  buffer->height = height;
  buffer->bytes_per_pixel = 4;
  buffer->pitch = buffer->width * buffer->bytes_per_pixel;
  buffer->size = buffer->bytes_per_pixel * (buffer->width * buffer->height);

  buffer->memory = mmap(NULL, buffer->size, PROT_READ | PROT_WRITE, MAP_PRIVATE);
  if(buffer->memory == MAP_FAILED){
    perror("mmap");
    return 1;
  }

}

internal void display_offscreen_buffer(Display* display, Window window, GC gc, XImage *image, int x1, int y1, int x2, int y2, unsigned int width, unsigned int height ){
  XPutImage(
      display,
      window,
      gc,
      image,
      x1, y1,
      x2, y2,
      width,
      height);
}
