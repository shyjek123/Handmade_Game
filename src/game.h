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
#define terabytes(amount) (megabytes(amount) * 1024)

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
// NOTE: not for final game, does block and does not protect against data loss
//
// struct debug_win32_fileIO_struct {
//   uint32_t size;
//   void *data;
// };
//
// internal debug_win32_fileIO_struct debug_win32_ReadFile(char *filename);
// internal bool debug_win32_WriteFile(char *filename, uint32_t size,
//                                     void *memory);
// internal void debug_win32_FreeFile(void *memory);
//
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
  int sample_count;
  int samples_per_second;
};

struct game_state_struct {
  double x;
  double y;

  int tonehz;
  int16_t toneVol;
};
struct game_memory_struct {
  bool isInitialized;

  void *permanent;
  uint64_t permanentSize;

  void *ram;
  uint64_t ramSize;
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
      game_button_state_struct l_shoulder;
      game_button_state_struct r_shoulder;
    };
  };
};

struct game_input_struct {
  game_controller_struct controllers[4];
};


// internal void game_Sound_Out(game_state_struct *game_state,
//                              game_sound_buffer_struct *sound);
//
// internal void game_Render_Colors(game_state_struct *game_state,
//                                  game_offscreen_buffer_struct buffer);
//
// internal void game_Sound_Out(game_state_struct *game_state,
//                              game_sound_buffer_struct *sound);
//
// internal void game_Update_Render(game_state_struct *game_memory,
//                                  game_offscreen_buffer_struct *buffer,
//                                  game_sound_buffer_struct *sound);
#define HANDMADE_H
#endif
