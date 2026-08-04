#if !defined(HANDMADE_H)
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <cstdio>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <x86intrin.h> // Required header for __rdtsc

#define internal static
#define local_persist static
#define global static

#define kilobytes(amount) ((amount) * 1024)
#define megabytes(amount) (kilobytes(amount) * 1024)
#define gigabytes(amount) (megabytes(amount) * 1024)
#define terabytes(amount) (gigabytes(amount) * 1024)

#define arraycount(arr) (sizeof(arr) / sizeof((arr)[0]))

#if HANDMADE_LINT
#define assert(exp)                                                            \
  if (!(exp)) {                                                                \
    *(int *)0 = 0;                                                             \
  }
#else
#define assert(exp)
#endif

inline uint32_t safe_uint64_truncate(uint64_t num) {
  assert(num <= 0xFFFFFFFF);
  return (uint32_t)num;
}

#if 1

//not for final game only debugging
struct debug_lin_fileIO_struct {
  uint32_t size;
  void *data;
};

internal debug_lin_fileIO_struct debug_lin_readfile(char *filename);
internal bool debug_lin_writefile(char *filename, uint32_t size,
                                    void *memory);
internal void debug_lin_freefile(void *memory, uint32_t *size);

#endif

struct game_offscreen_buffer_struct {
  void *memory;
  int width;
  int height;
  int bytes_per_pixel;
  int pitch;
};

struct game_sound_buffer_struct {
  int16_t *samples;
  uint32_t sample_count;
  int sample_rate;
};

struct game_state_struct {
  double x;
  double y;

  int tonehz;
  int16_t tonevol;
};

struct game_memory_struct {
  bool is_initialized;

  void *permanent;
  uint64_t permanent_size;

  void *ram;
  uint64_t ram_size;
};

struct game_button_state_struct {
  int half_transition_count;
  bool ended_down;
};

struct game_controller_struct {
  bool is_analog;

  int minX;
  int minY;

  int maxX;
  int maxY;

  int startX;
  int endX;

  int startY;
  int endY;

  union {
    game_button_state_struct buttons[6];
    struct {
      game_button_state_struct up;
      game_button_state_struct down;
      game_button_state_struct left;
      game_button_state_struct right;
      game_button_state_struct lshoulder;
      game_button_state_struct rshoulder;
    };
  };
};

struct game_input_struct {
  game_controller_struct controllers[4];
};


internal void game_sound_out(game_state_struct *game_state,
                             game_sound_buffer_struct *sound);

internal void game_render_colors(game_state_struct *game_state,
                                 game_offscreen_buffer_struct *buffer);

internal void game_update_render(game_memory_struct *game_memory,
                                 game_offscreen_buffer_struct *buffer,
                                 game_sound_buffer_struct *sound);
#define HANDMADE_H
#endif
