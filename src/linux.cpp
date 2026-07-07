#define ALSA_PCM_NEW_HW_PARAMS_API

#include "game.h"
#include <time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <alsa/asoundlib.h>

/* TODO: reimplement game, to work linux native
 *  priorities: 
 *  1. add sound output, (Sine Wave)
 *  for sound use stream type playback and RW_INTERLEAVED
 *  2. add game memory
 *  3. ensure everything adds up to what we have so far in olwin32
 *
 *    4. add remainder of key borad input
 *      (watch vidoes on 
 *        setup keyboard/controller input)
 *        basics of platform api design
 *        platform independent game memory, user input, sound, file IO, uniform input
 *    compartmentalize event handling, make into function
 *    add sound output (temporal sine wave sound)
 *    add xbox controller input
 *    have the screen move and sound buffer output sound according to keyboard and controller input
 *    setup game memory
 *    create game/platform layer
 *
 *
 */

//init audio vars here
//sourcevoice and lib structure
//
constexpr double PI = 3.14159265358979323846;
struct linux_sound_info_struct{
  uint16_t bits_per_sample;
  uint32_t samples_per_second;
  double tonehz;
  double volume;
  uint16_t size_in_cycles;
  uint32_t samples_per_cycle;
  uint32_t size_in_samples;
  uint32_t size_in_bytes;
};

struct linux_offscreen_buffer_struct{
  XImage *image;
  void *memory;
  int width;
  int height;
  int bytes_per_pixel;
  int pitch;
  size_t size;
};
global linux_offscreen_buffer_struct global_offscreen_buffer;

struct screen_dimensions_struct{
  int width;
  int height;
};

internal screen_dimensions_struct linux_get_window_dimensions(Display *display, Window window);
internal void linux_resize_image_section(Display *display, int screen, linux_offscreen_buffer_struct *buffer, int width, int height);
internal void linux_display_offscreen_buffer(Display* display, Window window, GC gc, XImage *image, int x, int y, unsigned int width, unsigned int height );
// internal void linux_handle_key_input(game_button_state_struct *new_key_state, bool key_status);
internal void linux_debug_print(const char* msg);
internal void linux_setup_audio(linux_sound_info_struct *linux_sound_info);
internal void linux_fill_soundbuffer(int16_t *buffer, linux_sound_info_struct *info_struct);
internal void game_Render_Colors(int x, int y,XImage *buffer);

global bool global_running;

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

  if (window){
    GC gc{}; 

    global_running = true;

    Atom wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wmDeleteWindow, 1);

    XMapWindow(display, window);
    XFlush(display);

    screen_dimensions_struct dims{};
    dims = linux_get_window_dimensions(display, window);
    linux_resize_image_section(display, screen, &global_offscreen_buffer, dims.width, dims.height);


    /*TODO: 
      setup sound structs and info
      allocate memory for audio
    */
    linux_sound_info_struct linux_sound_info = {};
    linux_sound_info.bits_per_sample = 16;
    linux_sound_info.samples_per_second = 44100;
    linux_sound_info.tonehz= 440.0;
    linux_sound_info.volume = 10.0;
    linux_sound_info.size_in_cycles = 10;
    linux_sound_info.samples_per_cycle = (int)(linux_sound_info.samples_per_second / linux_sound_info.tonehz);
    linux_sound_info.size_in_samples = linux_sound_info.samples_per_cycle * linux_sound_info.size_in_cycles;
    linux_sound_info.size_in_bytes = (linux_sound_info.size_in_samples * linux_sound_info.bits_per_sample)/8;

    linux_setup_audio(&linux_sound_info);

#if HANDMADE_DEV
    //void *base_address = (void *)terabytes((uint64_t)2);
#else
    //void *base_address = 0;
#endif

/*
 *TODO:
  1. setup and alloc game memory
  2. check if everything alloced
 * */


    struct timespec last_counter;
    clock_gettime(CLOCK_MONOTONIC, &last_counter);
    uint64_t last_cpu_clock = __rdtsc();
    
    game_input_struct input[2] = {};
    game_input_struct *new_input = &input[0];
    // game_input_struct *old_input = &input[1];
    //

    while (global_running) {
      /*TODO:
        * init game_controller/keyboard and zeroed input (read more about adding game controller/keyboard input)
        */
      game_controller_struct *keyboard_controller = &new_input->controllers[0];
      game_controller_struct zeroed = {};
      *keyboard_controller = zeroed;

      while(XPending(display)){

        XEvent event;
        XNextEvent(display, &event);

      //TODO: make sure all important msgs are being handled properly
        //handling events will be moved to separate event handler function
        switch (event.type) {
            case Expose:
              gc = XCreateGC(display, window, 0, NULL);
              dims = linux_get_window_dimensions(display, window);
              game_Render_Colors(x, y, global_offscreen_buffer.image);
              linux_display_offscreen_buffer(display, window, gc, global_offscreen_buffer.image, 0, 0, dims.width, dims.height);
              XFreeGC(display, gc);
              break; //2. No Window Has Input Focus

            case ConfigureNotify:
              //window resize
              width = event.xconfigure.width;
              height = event.xconfigure.height;

              //update frame buffer
              linux_resize_image_section(display, screen, &global_offscreen_buffer, width, height);
              break;

            case KeyPress: {
                // Check if the user pressed the Escape key to close the window
                // TODO: add KeyPress Handler function
              KeySym keysym = XLookupKeysym(&event.xkey, 0);
              if (keysym == XK_Escape) {
                  global_running = false;
              }
            
            
              // keysym = (uint32_t)msg.wParam;
              // bool was_down = (((msg.lParam & (1 << 30)) != 0));
              // bool is_down = (((msg.lParam & (1 << 31)) == 0));
              // if (was_down != is_down) {
              //   if (keysym == 'W') {
              //
              //     linux_handle_key_input(&keyboard_controller->l_shoulder,
              //                            is_down);
              //   } else if (keysym == 'A') {
              //
              //     linux_handle_key_input(&keyboard_controller->l_shoulder,
              //                            is_down);
              //     // Process the INS key.
              //   } else if (keysym == 'S') {
              //
              //     linux_handle_key_input(&keyboard_controller->l_shoulder,
              //                            is_down);
              //   } else if (keysym == 'D') {
              //
              //     linux_handle_key_input(&keyboard_controller->l_shoulder,
              //                            is_down);
              //   } else if (keysym == 'Q') {
              //     linux_handle_key_input(&keyboard_controller->l_shoulder,
              //                            is_down);
              //   } else if (keysym == 'E') {
              //     linux_handle_key_input(&keyboard_controller->r_shoulder,
              //                            is_down);
              //   } else if (keysym == XK_Left) {
              //     linux_handle_key_input(&keyboard_controller->left, is_down);
              //   } else if (keysym == XK_Right) {
              //     linux_handle_key_input(&keyboard_controller->right, is_down);
              //   } else if (keysym == XK_Up) {
              //     linux_handle_key_input(&keyboard_controller->up, is_down);
              //   } else if (keysym == XK_Down) {
              //     linux_handle_key_input(&keyboard_controller->down, is_down);
              //   } else if (keysym == XK_Escape) {
              //     global_running = false;
              //   }
              // } else if (keysym == XK_space) {
              //   linux_debug_print("SPACE PRESSED");
              // }
              //
              // bool alt_key_was_down = ((msg.lParam & (1 << 29)) != 0);
              // if (keysym == XK_F4 && alt_key_was_down) {
              //   global_running = false;
              // }
            }
            break;

            case ClientMessage:
                // Close the window if the user clicked the Window Manager close button
                if ((Atom)event.xclient.data.l[0] == wmDeleteWindow) {
                    global_running = false;
                }
                break;

            default:
                break;
        }
      }

        screen_dimensions_struct screen_dims = linux_get_window_dimensions(display, window);

        linux_resize_image_section(display, screen, &global_offscreen_buffer, screen_dims.width,
                              screen_dims.height);

        game_offscreen_buffer_struct game_offscreen_buffer{};
        game_offscreen_buffer.memory = global_offscreen_buffer.memory;
        game_offscreen_buffer.height = global_offscreen_buffer.height;
        game_offscreen_buffer.width = global_offscreen_buffer.width;
        game_offscreen_buffer.bytes_per_pixel =
            global_offscreen_buffer.bytes_per_pixel;
        game_offscreen_buffer.pitch = global_offscreen_buffer.pitch;

      GC gc = XCreateGC(display, window, 0, NULL);
      //TODO: the game buffer should be sent to game_Render_Colors, then that data needs to be copied to global.image, in separate game layer file
      game_Render_Colors(x, y, global_offscreen_buffer.image);
      linux_display_offscreen_buffer(display, window, gc, global_offscreen_buffer.image, 0, 0, game_offscreen_buffer.width, game_offscreen_buffer.height);
      XFreeGC(display, gc);

      uint64_t end_cpu_clock = __rdtsc();

      struct timespec end_counter;
      clock_gettime(CLOCK_MONOTONIC, &end_counter);

      int64_t counter_elapsed =
          (end_counter.tv_sec - last_counter.tv_sec) * 1000000000LL +
          (end_counter.tv_nsec - last_counter.tv_nsec);

      uint64_t cycles_elapsed =
          end_cpu_clock - last_cpu_clock;

      double ms_per_frame =
          (double)counter_elapsed / 1000000.0;

      double fps =
          1000000000.0 / (double)counter_elapsed;

      double seconds =
          (double)counter_elapsed / 1000000000.0;

      double mega_cycles_per_second =
          ((double)cycles_elapsed / seconds) / 1000000.0;

      const char *format = "%.02fms/f %.02ff/s %.02fmc/s\n";


      char buffer[256];
      snprintf(
          buffer,
          sizeof(buffer),
          format,
          ms_per_frame,
          fps,
          mega_cycles_per_second);

      linux_debug_print(buffer);

      last_counter = end_counter;
      last_cpu_clock = end_cpu_clock;    

         }
  }
   if(global_offscreen_buffer.image){
          global_offscreen_buffer.image->data = NULL;
          XDestroyImage(global_offscreen_buffer.image);
        }


    XDestroyWindow(display, window);
    XCloseDisplay(display);

  return 0;
}


internal screen_dimensions_struct linux_get_window_dimensions(Display *display, Window window){
  screen_dimensions_struct result;
  XWindowAttributes attrs;
  XGetWindowAttributes(display, window, &attrs);

  int width  = attrs.width;
  int height = attrs.height;
  result.width = width;
  result.height = height;

  return result;
}

internal void linux_resize_image_section(Display *display, int screen, linux_offscreen_buffer_struct *buffer, int width, int height){


  if(buffer->memory){
    munmap(buffer->memory, buffer->size);
  }

  if(global_offscreen_buffer.image){
    global_offscreen_buffer.image->data = NULL;
    XDestroyImage(global_offscreen_buffer.image);
  }


  buffer->width = width;
  buffer->height = height;

  buffer->bytes_per_pixel = 4;
  buffer->pitch = buffer->width * buffer->bytes_per_pixel;
  buffer->size = buffer->bytes_per_pixel * (buffer->width * buffer->height);


  buffer->memory = mmap(NULL, buffer->size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if(buffer->memory == MAP_FAILED){
    perror("mmap");
    exit(1);
  }

  buffer->image = XCreateImage(display, DefaultVisual(display, screen), DefaultDepth(display, screen), ZPixmap, 0, (char *)buffer->memory, buffer->width, buffer->height, 32, buffer->pitch);
}

internal void linux_display_offscreen_buffer(Display* display, Window window, GC gc, XImage *image, int x, int y, unsigned int width, unsigned int height ){


  XPutImage(
      display,
      window,
      gc,
      image,
      x, y,
      0, 0,
      width,
      height);
}

internal void linux_debug_print(const char* msg) {
    fprintf(stderr, "[DEBUG] %s\n", msg);
    fflush(stderr); // Force it to print immediately
}

// internal void linux_handle_key_input(game_button_state_struct *new_key_state,
//                                      bool key_status){
//   new_key_state->ended_down = key_status;
//   ++new_key_state->half_transition_count;
//
// }


internal void linux_setup_audio(linux_sound_info_struct *linux_sound_info){
  long num_loops;
  int result;
  int size;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;
  unsigned int period_time;
  int dir;
  snd_pcm_uframes_t frames;
//  int16_t *buffer;

  /* Open PCM device for playback. */
  result = snd_pcm_open(&handle, "default",
                    SND_PCM_STREAM_PLAYBACK, 0);
  if (result < 0) {
    fprintf(stderr,
            "unable to open pcm device: %s\n",
            snd_strerror(result));
    exit(1);
  }

  /* Allocate a hardware parameters object. */
  snd_pcm_hw_params_alloca(&params);

  /* Fill it in with default period_timeues. */
  snd_pcm_hw_params_any(handle, params);

  /* Set the desired hardware parameters. */

  /* Interleaved mode */
  snd_pcm_hw_params_set_access(handle, params,
                      SND_PCM_ACCESS_RW_INTERLEAVED);

  /* Signed 16-bit little-endian format */
  snd_pcm_hw_params_set_format(handle, params,
                              SND_PCM_FORMAT_S16_LE);

  /* Two channels (stereo) */
  snd_pcm_hw_params_set_channels(handle, params, 2);

  /* 44100 bits/second sampling rate (CD quality) */
  period_time = linux_sound_info->samples_per_second;
  snd_pcm_hw_params_set_rate_near(handle, params,
                                  &period_time, &dir);

  frames = linux_sound_info->samples_per_cycle;
  snd_pcm_hw_params_set_period_size_near(handle,
                              params, &frames, &dir);

  /* Write the parameters to the driver */
  result = snd_pcm_hw_params(handle, params);
  if (result < 0) {
    fprintf(stderr,
            "unable to set hw parameters: %s\n",
            snd_strerror(result));
    exit(1);
  }

  /* Use a buffer large enough to hold one period */
  snd_pcm_hw_params_get_period_size(params, &frames,
                                    &dir);
//  size = frames * 4; /* 2 bytes/sample, 2 channels */
  size = 88200;
  int16_t samples[size];
  //buffer = (int16_t*) malloc(size);

  /* We want to loop for 5 seconds */
  snd_pcm_hw_params_get_period_time(params,
                                    &period_time, &dir);
  /* 5 seconds in microseconds divided by
   * period time */
  num_loops = 5000000 / period_time;

  while (num_loops > 0) {
    num_loops--;
      //add fill soundbuffer function
    linux_fill_soundbuffer(samples, linux_sound_info);
    // if (result == 0) {
    //   fprintf(stderr, "end of file on input\n");
    //   break;
    // } else if (result != size) {
    //   fprintf(stderr,
    //           "short read: read %d bytes\n", result);
    // }
    
    snd_pcm_prepare(handle);
    result = snd_pcm_writei(handle, samples, frames);
    
    /*
     * check for EAGAIN error code, means ring buf is full
     * quee another buf only if result != EAGAIN
     */
    if (result == -EPIPE) {
      /* EPIPE means underrun */
      fprintf(stderr, "underrun occurred\n");
      snd_pcm_writei(handle, samples, frames);
    } else if (result < 0) {
      fprintf(stderr,
              "error from writei: %s\n",
              snd_strerror(result));
    }  else if (result != (int)frames) {
      fprintf(stderr,
              "short write, write %d frames\n", result);
    }

  }

  snd_pcm_start(handle);


  snd_pcm_drain(handle);
  snd_pcm_close(handle);
//  free(buffer);
}

internal void linux_fill_soundbuffer(int16_t *buffer, linux_sound_info_struct *info_struct) {

   local_persist double phase{};

   int16_t *bufferOut = buffer;
   double phaseIncrement =
       (2.0 * 3.14159265358979323846 * 1.0f /
        (info_struct->samples_per_second/ (float)info_struct->tonehz));

   for (uint32_t bufferIndex = 0; bufferIndex < info_struct->samples_per_second; ++bufferIndex) {
     double sineVal = sin(phase);
     int16_t sampleVal = (int16_t)(sineVal * info_struct->volume);

     *bufferOut++ = sampleVal;
     *bufferOut++ = sampleVal;

     phase += phaseIncrement;
     if (phase > (2.0 * 3.14159265358979323846))
       phase -= (2.0 * 3.14159265358979323846);
   }
}

internal void game_Render_Colors(int x, int y,XImage *buffer) {
  uint8_t *Row = (uint8_t *)buffer->data;
  uint32_t *Pixel;
  for (int pixelY = 0; pixelY < buffer->height; ++pixelY) {
    Pixel = (uint32_t *)Row;
    for (int pixelX = 0; pixelX < buffer->width; ++pixelX) {
      uint8_t Green = (uint8_t)(pixelX + x);
      uint8_t Blue = (uint8_t)(pixelY + y);
      *Pixel++ = (Green << 8) | Blue;
    }
    Row += buffer->bytes_per_line;
  }
}


