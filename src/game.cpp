#include "game.h"
// internal void game_Render_Colors(game_state_struct *game_state,
//                                  game_offscreen_buffer_struct *buffer) {
//   uint8_t *Row = (uint8_t *)buffer->memory;
//   uint32_t *Pixel;
//   for (int pixelY = 0; pixelY < buffer->height; ++pixelY) {
//     Pixel = (uint32_t *)Row;
//     for (int pixelX = 0; pixelX < buffer->width; ++pixelX) {
//       uint8_t Green = (uint8_t)(pixelX + game_state->x);
//       uint8_t Blue = (uint8_t)(pixelY + game_state->y);
//       *Pixel++ = (Green << 8) | Blue;
//     }
//     Row += buffer->pitch;
//   }
// }
//
// internal void game_Sound_Out(game_state_struct *game_state,
//                              game_sound_buffer_struct *sound) {
//
//   local_persist double phase{};
//
//   int16_t *bufferOut = sound->samples;
//   double phaseIncrement =
//       (2.0 * 3.14159265358979323846 * 1.0f /
//        (sound->samplesPerSecond / (float)game_state->tonehz));
//
//   for (int bufferIndex = 0; bufferIndex < sound->sampleCount; ++bufferIndex) {
//     double sineVal = sin(phase);
//     int16_t sampleVal = (int16_t)(sineVal * game_state->toneVol);
//
//     *bufferOut++ = sampleVal;
//     *bufferOut++ = sampleVal;
//
//     phase += phaseIncrement;
//     if (phase > (2.0 * 3.14159265358979323846))
//       phase -= (2.0 * 3.14159265358979323846);
//   }
// }
//
// internal void game_Update_Render(game_memory_struct *game_memory,
//                                  game_offscreen_buffer_struct *buffer,
//                                  game_sound_buffer_struct *sound) {
//
//   game_state_struct *game_state = (game_state_struct *)game_memory->permanent;
//
//   assert(sizeof(game_state) <= game_memory->permanentSize);
//
//   if (!game_memory->isInitialized) {
//     game_memory->isInitialized = true;
//     game_state->x = 0;
//     game_state->y = 0;
//     game_state->tonehz = 256;
//     game_state->toneVol = 30000;
//
//     // NOTE: testing file IO
//     //  must transfer to linux File IO
//     debug_win32_fileIO_struct file_info =
//         debug_win32_ReadFile((char *)__FILE__);
//     if (file_info.data) {
//       debug_win32_WriteFile(
//           (char *)"C:\\Users\\sebas\\source\\projects\\Handmade_"
//                   "Hero\\data\\test."
//                   "txt",
//           file_info.size, file_info.data);
//     }
//   }
//
//   game_Sound_Out(game_state, sound);
//   game_Render_Colors(game_state, buffer);
// }
