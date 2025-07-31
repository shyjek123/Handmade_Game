#if !defined(HANDMADE_H)
#include <math.h>
#include <stdint.h>
#define internal static
#define local_persist static
#define global static

struct screen_dimensions_struct {
  int width;
  int height;
};

struct game_offscreen_buffer_struct {
  void *memory;
  int width;
  int height;
  int bytesPerPixel;
  int pitch;
};

struct game_sound_buffer_struct {
  int16_t *samples;
  int sampleCount;
  int samplesPerSecond;
};

internal void game_Render_Colors(game_offscreen_buffer_struct buffer, int x,
                                 int y, game_sound_buffer_struct *sound);
internal void game_Sound_Out(game_sound_buffer_struct *sound);
internal void game_Update_Render();
#define HANDMADE_H
#endif
