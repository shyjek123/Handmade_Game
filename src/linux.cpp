#define ALSA_PCM_NEW_HW_PARAMS_API

#include "game.h"
#include "game.cpp"
#include <malloc.h>
#include <sys/stat.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <alsa/asoundlib.h>

/* TODO: reimplement game, to work linux native
 *  priorities: 
 *  1. implement controller input look at SDL2 library
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

constexpr double PI = 3.14159265358979323846;
struct lin_sound_info_struct{
  const char *device;
  snd_pcm_t *handle;
  uint16_t bits_per_sample;
  uint32_t sample_rate;
  double tonehz;
  double volume;
  uint16_t size_in_cycles;
  uint32_t samples_per_cycle;
  uint32_t size_in_samples;
  uint32_t size_in_bytes;
  uint16_t nchannels;
};


struct lin_offscreenbuf{
  XImage *image;
  void *memory;
  size_t size;
  int width;
  int height;
  int bytes_per_pixel;
  int pitch;
};
global lin_offscreenbuf global_offscreen_buffer;

struct screen_dimensions_struct{
  int width;
  int height;
};

internal screen_dimensions_struct linux_get_window_dimensions(Display *display, Window window);
internal void linux_resize_image_section(Display *display, int screen, lin_offscreenbuf *buffer, int width, int height);
internal void linux_display_offscreen_buffer(Display* display, Window window, GC gc, XImage *image, int x, int y, unsigned int width, unsigned int height );
internal void linux_handle_key_input(game_button_state_struct *new_key_state, bool key_status);
internal void linux_debug_print(const char* msg);
internal void lin_fill_soundbuffer(game_sound_buffer_struct *src_sound_buf, int16_t *dst_sound_buf);

internal bool debug_lin_writefile(char* filename, uint32_t size, void *memory);
internal debug_lin_fileIO_struct debug_lin_readfile(char *filename);
internal void debug_lin_freefile(void *memory, uint32_t *size);

global bool global_running;

//FIX: audio stops when interating with the window, it should be a constant stream
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
    attributes.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask | KeyReleaseMask;

    struct timespec last_counter;
    clock_gettime(CLOCK_MONOTONIC, &last_counter);

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
    gc = XCreateGC(display, window, 0, NULL);

    global_running = true;

    Atom wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wmDeleteWindow, 1);

    XMapWindow(display, window);
    XFlush(display);

    screen_dimensions_struct dims{};
    dims = linux_get_window_dimensions(display, window);
    linux_resize_image_section(display, screen, &global_offscreen_buffer, dims.width, dims.height);


    lin_sound_info_struct linux_sound_info = {};
    linux_sound_info.device = "default";
    linux_sound_info.bits_per_sample = 16;
    linux_sound_info.sample_rate = 44100;
    linux_sound_info.tonehz= 256.0;
    linux_sound_info.volume = 10.0;
    linux_sound_info.size_in_cycles = 10;
    linux_sound_info.samples_per_cycle = (int)(linux_sound_info.sample_rate / linux_sound_info.tonehz);
    linux_sound_info.size_in_samples = linux_sound_info.samples_per_cycle * linux_sound_info.size_in_cycles;
    linux_sound_info.size_in_bytes = (linux_sound_info.size_in_samples * linux_sound_info.bits_per_sample)/8;
    linux_sound_info.nchannels = 2;

    snd_pcm_status_t *pcm_status;
    snd_pcm_status_alloca(&pcm_status);

    char buffer[256];
    int err;
    if((err=snd_pcm_open(&linux_sound_info.handle, linux_sound_info.device, SND_PCM_STREAM_PLAYBACK, 0)) < 0){
      const char *format = "pcm_open error: %s\n";
      snprintf(
            buffer,
            sizeof(buffer),
            format,
            snd_strerror(err));
      linux_debug_print(buffer);
      exit(1);
    }
    if((err = snd_pcm_set_params(linux_sound_info.handle,
                                 SND_PCM_FORMAT_S16_LE,
                                 SND_PCM_ACCESS_RW_INTERLEAVED,
                                 linux_sound_info.nchannels,
                                 linux_sound_info.sample_rate,
                                 1,     //softresample, uses software if hardware cant compensate rate
                                 30000// target latency
                                 ))<0){
      const char *format = "snd_pcm_set_params error: %s\n";
      snprintf(
            buffer,
            sizeof(buffer),
            format,
            snd_strerror(err));
      linux_debug_print(buffer);
      exit(1);
    }

    int16_t *main_audio = (int16_t *)mmap(NULL, 88200, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(main_audio == MAP_FAILED){
      linux_debug_print("error with mmap allocating main audio");
      exit(1);
    }


#if HANDMADE_DEV
    void *base_address = (void *)terabytes((uint64_t)2);
#else
    void *base_address = 0;
#endif

    game_memory_struct game_memory = {};
    game_memory.permanent_size = megabytes(64);
    game_memory.ram_size = gigabytes((uint16_t)1);

    uint64_t total_memory = game_memory.permanent_size + game_memory.ram_size;
    game_memory.permanent = mmap(base_address, total_memory, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(game_memory.permanent == MAP_FAILED){
      linux_debug_print("error with mmap allocating game_memory.permanent");
      exit(1);
    }
    game_memory.ram = ((uint8_t *)game_memory.permanent + game_memory.permanent_size);
    
    uint64_t last_cpu_clock = __rdtsc();
    if(main_audio && game_memory.permanent && game_memory.ram){
      game_input_struct input[2] = {};
      game_input_struct *new_input = &input[0];
      game_input_struct *old_input = &input[1];

      while (global_running) {

        game_controller_struct *keyboard_controller = &new_input->controllers[0];

        for(uint8_t i = 0; i < arraycount(keyboard_controller->buttons); ++i){
          keyboard_controller->buttons[i].ended_down = old_input->controllers[0].buttons[i].ended_down;
          keyboard_controller->buttons[i].half_transition_count = 0;
        }


        while(XPending(display)){

          XEvent event;
          XNextEvent(display, &event);

        //TODO: make sure all important msgs are being handled properly
        //handling events will be moved to separate event handler function
          switch (event.type) {
              case Expose:
                dims = linux_get_window_dimensions(display, window);
                linux_display_offscreen_buffer(display, window, gc, global_offscreen_buffer.image, 0, 0, dims.width, dims.height);
                break;

              case ConfigureNotify:
                //window resize
                width = event.xconfigure.width;
                height = event.xconfigure.height;

                //update frame buffer
                linux_resize_image_section(display, screen, &global_offscreen_buffer, width, height);
                break;
              case KeyRelease:
              case KeyPress:
              {
                KeySym keysym = XLookupKeysym(&event.xkey, 0);
                game_button_state_struct *obutton = 0;
                game_button_state_struct *nbutton = 0;
                bool is_down = (event.type == KeyPress);
                bool was_down = false;
                switch (keysym){
                  case XK_w:
                    obutton = &old_input->controllers[0].up;
                    nbutton = &keyboard_controller->up;
                    break;
                  case XK_a:
                    obutton = &old_input->controllers[0].left;
                    nbutton = &keyboard_controller->left;
                    break;
                  case XK_s:
                    obutton = &old_input->controllers[0].down;
                    nbutton = &keyboard_controller->down;
                    break;
                  case XK_d:
                    obutton = &old_input->controllers[0].right;
                    nbutton = &keyboard_controller->right;
                    break;
                  case XK_q:
                    obutton = &old_input->controllers[0].lshoulder;
                    nbutton = &keyboard_controller->lshoulder;
                    break;
                  case XK_e:
                    obutton = &old_input->controllers[0].rshoulder;
                    nbutton = &keyboard_controller->rshoulder;
                    break;
                  case XK_Left:
                    obutton = &old_input->controllers[0].left;
                    nbutton = &keyboard_controller->left;
                    break;
                  case XK_Right:
                    obutton = &old_input->controllers[0].right;
                    nbutton = &keyboard_controller->right;
                    break;
                  case XK_Up:
                    obutton = &old_input->controllers[0].up;
                    nbutton = &keyboard_controller->up;
                    break;
                  case XK_Down:
                    obutton = &old_input->controllers[0].down;
                    nbutton = &keyboard_controller->down;
                    break;
                  case XK_space:
                    linux_debug_print("SPACE was pressed");
                    break;
                  default:
                    break;
                }

                if(obutton){
                  was_down = obutton->ended_down;
                  if(is_down != was_down)
                    linux_handle_key_input(nbutton, is_down);
                }

                bool alt_was_down = (event.xkey.state & Mod1Mask) != 0;
                if(keysym == XK_F4 && alt_was_down)
                  global_running = false;
                if(keysym == XK_Escape && is_down)
                  global_running = false;
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


          // // Gamepad input handling
          // for (DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT;
          //      controllerIndex++) {
          //   XINPUT_STATE input_state{};
          //   ZeroMemory(&input_state, sizeof(XINPUT_STATE));
          //   if (XinputGetState(controllerIndex, &input_state) ==
          //       ERROR_SUCCESS) {
          //     // Controller is connected
          //     XINPUT_GAMEPAD *pad = &input_state.Gamepad;
          //     // do actions based on input received
          //     // NOTE: Maybe have to end up handling Deadzone
          //     // MSDN:
          //     // https: //
          //     //    learn.microsoft.com/en-us/windows/win32/xinput/getting-started-with-xinput#getting-controller-state
          //
          //   } else {
          //     // Controller is not connected
          //   }
          // }
          //
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
      
        int16_t samples[88200];
        game_sound_buffer_struct game_sound_buffer = {};
        game_sound_buffer.sample_rate = linux_sound_info.sample_rate;
        game_sound_buffer.sample_count = (linux_sound_info.sample_rate / 60);
        game_sound_buffer.samples = samples;

        snd_pcm_uframes_t buffer_size;
        snd_pcm_uframes_t period_size;
        snd_pcm_get_params(linux_sound_info.handle, &buffer_size, &period_size);

        snd_pcm_uframes_t frames_avail;
        snd_pcm_sframes_t frames_written;
        uint8_t buffer_queue;
        int err;


        game_update_render(&game_memory, &game_offscreen_buffer, &game_sound_buffer);

        if((err = snd_pcm_status(linux_sound_info.handle, pcm_status)) == 0){

          frames_avail = snd_pcm_status_get_avail(pcm_status);
          buffer_queue = buffer_size - frames_avail;

          if(buffer_queue < buffer_size && frames_avail > 0){
            lin_fill_soundbuffer(&game_sound_buffer, main_audio);

            frames_written = snd_pcm_writei(linux_sound_info.handle, main_audio, (linux_sound_info.sample_rate/60));
            if(frames_written < 0){
              snd_pcm_recover(linux_sound_info.handle, frames_written, 0);
              const char *format = "writei error: %s\n";
                  snprintf(
                      buffer,
                      sizeof(buffer),
                      format,
                      snd_strerror(frames_written));
                  linux_debug_print(buffer);
                }


            }
        }else{
            const char *format = "pcm_status != 0, code: %d\n";
            snprintf(buffer, sizeof(buffer), format, snd_strerror(err), frames_written);
            linux_debug_print(buffer);
        }

        linux_display_offscreen_buffer(display, window, gc, global_offscreen_buffer.image, 0, 0, game_offscreen_buffer.width, game_offscreen_buffer.height);

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

        game_input_struct *tmp = new_input;
        new_input= old_input;
        old_input = tmp;
        }
    }

    XFreeGC(display, gc);
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

internal void linux_resize_image_section(Display *display, int screen, lin_offscreenbuf *buffer, int width, int height){


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
  //TODO: add option to add args, similar to main, argc, argv
    fprintf(stderr, "[DEBUG] %s\n", msg);
    fflush(stderr); // Force it to print immediately
}

internal void linux_handle_key_input(game_button_state_struct *new_key_state,
                                     bool key_status){
  new_key_state->ended_down = key_status;
  ++new_key_state->half_transition_count;
}

internal void debug_lin_freefile(void *memory, uint32_t *size){
  munmap(memory, *size);
}

internal debug_lin_fileIO_struct debug_lin_readfile(char *filename){
  int fd;
  struct stat st;
  debug_lin_fileIO_struct fileio{};

  fd = open(filename, O_RDONLY);

  if(fd == -1){
    //TODO: logging
    linux_debug_print("error with open() in readfile");
    exit(1);
  }

  if(fstat(fd, &st) == -1) {
    linux_debug_print("fstat failed"); 
    close(fd);
    exit(1);
  }

  fileio.size = (size_t)st.st_size;
  if(!(fileio.size == 0)){
    fileio.data= mmap(NULL, fileio.size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  }else{
    linux_debug_print("file.size in debug_lin_readfile, <= 0");
  }

  if(fileio.data != MAP_FAILED){
    fileio.size = read(fd, fileio.data, fileio.size);
    if(fileio.size == (uint32_t)-1){
      linux_debug_print("read failed");
      close(fd);
      debug_lin_freefile(fileio.data, &fileio.size);
      fileio.data = 0;
      exit(1);
    }
  }else{
    linux_debug_print("malloc for file failed");
    close(fd);
    exit(1);
  }

  close(fd);

  return fileio;
}

internal bool debug_lin_writefile(char* filename, uint32_t size, void *memory){
  int fd;
  bool result = false;
  ssize_t bytes_written;

  fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
  if(fd == -1){
    //TODO: possible error in game msg to the user, does not exit program
    const char *format = "write file open() error: %d\n";
    char buffer[256];
    snprintf(
          buffer,
          sizeof(buffer),
          format,
          errno);
    linux_debug_print(buffer);
    exit(1);
  }
      bytes_written = write(fd, memory, size);
  if(bytes_written == -1){
  //logging
    linux_debug_print("error with write() in writefile");
  }
  result = (bytes_written == size);

  if(fd)
    close(fd);

  return result;
}


internal void lin_fill_soundbuffer(game_sound_buffer_struct *src_sound_buf,
                                     int16_t *dst_sound_buf) {

  int16_t *dst = dst_sound_buf;

  for (uint32_t bufferIndex = 0; bufferIndex < src_sound_buf->sample_count*2;
       ++bufferIndex) {

    *dst++ = src_sound_buf->samples[bufferIndex];
  }
}

