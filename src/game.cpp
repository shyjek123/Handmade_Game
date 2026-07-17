#include "game.h"

internal void game_render_colors(game_state_struct *game_state,
                                 game_offscreen_buffer_struct *buffer) {
  uint8_t *Row = (uint8_t *)buffer->memory;
  uint32_t *Pixel;
  for (int pixelY = 0; pixelY < buffer->height; ++pixelY) {
    Pixel = (uint32_t *)Row;
    for (int pixelX = 0; pixelX < buffer->width; ++pixelX) {
      uint8_t Green = (uint8_t)(pixelX + game_state->x);
      uint8_t Blue = (uint8_t)(pixelY + game_state->y);
      *Pixel++ = (Green << 8) | Blue;
    }
    Row += buffer->pitch;
  }
}

internal void game_sound_out(game_state_struct *game_state,
                             game_sound_buffer_struct *sound) {

  local_persist double phase{};

  int16_t *bufferOut = sound->samples;
  double phaseIncrement =
      (2.0 * 3.14159265358979323846 * 1.0f /
       (sound->samples_per_second/ (float)game_state->tonehz));

  for (uint32_t bufferIndex = 0; bufferIndex < sound->sample_count; ++bufferIndex) {
    double sineVal = sin(phase);
    int16_t sampleVal = (int16_t)(sineVal * game_state->tonevol);

    *bufferOut++ = sampleVal;
    *bufferOut++ = sampleVal;

    phase += phaseIncrement;
    if (phase > (2.0 * 3.14159265358979323846))
      phase -= (2.0 * 3.14159265358979323846);
  }
}

internal void game_update_render(game_memory_struct *game_memory,
                                 game_offscreen_buffer_struct *buffer,
                                 game_sound_buffer_struct *sound) {

  game_state_struct *game_state = (game_state_struct *)game_memory->permanent;

  assert(sizeof(game_state) <= game_memory->permanent_size);

  if (!game_memory->is_initialized) {
    game_memory->is_initialized = true;
    game_state->x = 0;
    game_state->y = 0;
    game_state->tonehz = 256;
    game_state->tonevol = 30000;

    debug_lin_fileIO_struct file_info =
        debug_lin_readfile((char *)__FILE__);
    if (file_info.data) {
      debug_lin_writefile(
          (char *)"/home/chillguy/projects/Handmade_Game/data",
          file_info.size, file_info.data);
    }
  }

  game_sound_out(game_state, sound);
  game_render_colors(game_state, buffer);
}
