#include "game.h"
struct screen_dimensions {
  int width;
  int height;
};

struct game_offscreen_buffer {
  void *memory;
  int width;
  int height;
  int bytesPerPixel;
  int pitch;
};

internal void renderColors(game_offscreen_buffer buffer, int x, int y) {
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

internal void game_update_render(game_offscreen_buffer *buffer, int x, int y) {

  renderColors(*buffer, x, y);
}
