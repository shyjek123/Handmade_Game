#if !defined(HANDMADE_H)
#include <stdint.h>

#define internal static
#define local_persist static
#define global static

struct screen_dimensions;
struct game_offscreen_buffer;

internal void renderColors(game_offscreen_buffer buffer, int x, int y);
internal void game_update_render();
#define HANDMADE_H
#endif
