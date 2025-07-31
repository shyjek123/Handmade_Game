#include "game.h"

internal void game_Render_Colors(game_offscreen_buffer_struct buffer, int x,
                                 int y) {
  uint8_t *Row = (uint8_t *)buffer.memory;
  uint32_t *Pixel;
  for (int pixelY = 0; pixelY < buffer.height; ++pixelY) {
    Pixel = (uint32_t *)Row;
    for (int pixelX = 0; pixelX < buffer.width; ++pixelX) {
      uint8_t Green = pixelX + x;
      uint8_t Blue = pixelY + y;
      *Pixel++ = (Green << 8) | Blue;
    }
    Row += buffer.pitch;
  }
}

internal void game_Sound_Out(game_sound_buffer_struct *sound) {
  local_persist double phase{};

  int tonehz = 256;
  int16_t toneVol = 3000;
  int16_t *bufferOut = sound->samples;
  double phaseIncrement =
      (2.0 * 3.14159265358979323846 * tonehz) / (double)sound->samplesPerSecond;

  for (int bufferIndex = 0; bufferIndex < sound->sampleCount; ++bufferIndex) {
    float sineVal = sin(phase);
    int16_t sampleVal = (int16_t)(sineVal * toneVol);

    *bufferOut++ = sampleVal;
    *bufferOut++ = sampleVal;

    phase += phaseIncrement;
    if (phase > (2.0 * 3.14159265358979323846))
      phase -= (2.0 * 3.14159265358979323846);
  }
}

internal void game_Update_Render(game_offscreen_buffer_struct *buffer, int x,
                                 int y, game_sound_buffer_struct *sound) {
  game_Sound_Out(sound);
  game_Render_Colors(*buffer, x, y);
}
