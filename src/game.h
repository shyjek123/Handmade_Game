/*TODO: add libraries so far, linux equivalents
  malloc.h
  math.h
  stdint.h
  OS native stuff lib
  audo input lib
  input library
*/
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>

struct offscreen_buffer{
  void* memory;
  int width;
  int height;
  int bytesPerPixel;
  int pitch;
};
global offscreen_buffer global_offscreenbuffer;
struct screen_dimensions_struct {
  int width;
  int height;
};

//put a window handle in and get the window dimensions back
internal screen_dimensions_struct get_window_dimensions();

//resize the window
internal void resize_window();

//display buffer to the window
internal void display_offscreenbuffer();


//constexpr double PI = 3.14159265358979323846;
// struct win32_sound_info_struct {
//   uint16_t bitsPerSample;
//   uint32_t samplesPerSecond;
//   double toneHz;
//   double volume;
//   uint16_t sizeInCycles;
//   uint32_t samplesPerCycle;
//   uint32_t sizeInSamples;
//   uint32_t sizeInBytes;
// };
//
// struct Win32offScreenBuf {
//   BITMAPINFO info;
//   void *memory;
//   int width;
//   int height;
//   int bytesPerPixel;
//   int pitch;
// };
// global Win32offScreenBuf global_offscreenBuffer;
// struct screen_dimensions_struct {
//   int width;
//   int height;
// };
//
// typedef HRESULT(WINAPI *XAUDIO2CREATE_def)(IXAudio2 **ppXAudio2, UINT32 Flags,
//                                            XAUDIO2_PROCESSOR XAudio2Processor);
// typedef DWORD(WINAPI *XINPUT_GET_STATE_def)(DWORD dwUserIndex,
//                                             XINPUT_STATE *pState);
// global XINPUT_GET_STATE_def XinputGetState;
//
// LRESULT CALLBACK win32_Window_Proc(HWND hwnd, UINT uMsg, WPARAM wParam,
//                                    LPARAM lParam);
//
// internal screen_dimensions_struct win32_Get_Window_Dimensions(HWND window);
//
// internal void win32_Resize_Dib_Sect(Win32offScreenBuf *buffer, int width,
//                                     int height);
//
// internal void win32_Fill_SoundBuffer(game_sound_buffer_struct *src_sound_buf,
//                                      int16_t *dst_sound_buf);
//
// internal int win32_Init_Xaudio2();
//
// internal void win32_Load_Libs();
//
// internal void win32_Display_Offscreen_Buffer(HDC dc, Win32offScreenBuf *buffer,
//                                              int x, int y, int width,
//                                              int height);
//
// internal void win32_Handle_Key_Input(game_button_state_struct *new_key_state,
//                                      bool key_state);

