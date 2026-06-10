#include "game.cpp"
#include <malloc.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <windows.h>
#include <xaudio2.h>
#include <xinput.h>

#define internal static
#define local_persist static
#define global static

global bool global_running;

//  INFO: AUDIO VARS
global IXAudio2 *pXAudio2{0};
global IXAudio2SourceVoice *source_voice{0};

constexpr double PI = 3.14159265358979323846;
struct win32_sound_info_struct {
  uint16_t bitsPerSample;
  uint32_t samplesPerSecond;
  double toneHz;
  double volume;
  uint16_t sizeInCycles;
  uint32_t samplesPerCycle;
  uint32_t sizeInSamples;
  uint32_t sizeInBytes;
};

struct Win32offScreenBuf {
  BITMAPINFO info;
  void *memory;
  int width;
  int height;
  int bytesPerPixel;
  int pitch;
};
global Win32offScreenBuf global_offscreenBuffer;
struct screen_dimensions_struct {
  int width;
  int height;
};

typedef HRESULT(WINAPI *XAUDIO2CREATE_def)(IXAudio2 **ppXAudio2, UINT32 Flags,
                                           XAUDIO2_PROCESSOR XAudio2Processor);
typedef DWORD(WINAPI *XINPUT_GET_STATE_def)(DWORD dwUserIndex,
                                            XINPUT_STATE *pState);
global XINPUT_GET_STATE_def XinputGetState;

LRESULT CALLBACK win32_Window_Proc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam);

internal screen_dimensions_struct win32_Get_Window_Dimensions(HWND window);

internal void win32_Resize_Dib_Sect(Win32offScreenBuf *buffer, int width,
                                    int height);

internal void win32_Fill_SoundBuffer(game_sound_buffer_struct *src_sound_buf,
                                     int16_t *dst_sound_buf);

internal int win32_Init_Xaudio2();

internal void win32_Load_Libs();

internal void win32_Display_Offscreen_Buffer(HDC dc, Win32offScreenBuf *buffer,
                                             int x, int y, int width,
                                             int height);

internal void win32_Handle_Key_Input(game_button_state_struct *new_key_state,
                                     bool key_state);

int CALLBACK WinMain(HINSTANCE inst, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
                     int nCmdShow) {
  const char CLASS_NAME[] = "HandmadeWndClass";

  WNDCLASS wc = {};

  wc.style = CS_VREDRAW | CS_HREDRAW;
  wc.lpfnWndProc = win32_Window_Proc;
  wc.hInstance = inst;
  wc.lpszClassName = CLASS_NAME;

  LARGE_INTEGER frequency_res;
  QueryPerformanceFrequency(&frequency_res);
  int64_t frequency = frequency_res.QuadPart;

  if (RegisterClass(&wc)) {
    HWND windHandle = CreateWindowEx(

        0, wc.lpszClassName, "Handmade Hero", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, inst,
        0);
    if (windHandle) {
      global_running = true;

      screen_dimensions_struct dimensions =
          win32_Get_Window_Dimensions(windHandle);
      win32_Resize_Dib_Sect(&global_offscreenBuffer, dimensions.width,
                            dimensions.height);

      win32_Load_Libs();

      // INFO: audio stuff
      win32_sound_info_struct win32_sound_info = {};
      win32_sound_info.bitsPerSample = 16;
      win32_sound_info.samplesPerSecond = 44100;
      win32_sound_info.toneHz = 440.0;
      win32_sound_info.volume = 10.0;
      win32_sound_info.sizeInCycles = 10;
      win32_sound_info.samplesPerCycle =
          (int)(win32_sound_info.samplesPerSecond / win32_sound_info.toneHz);
      win32_sound_info.sizeInSamples =
          win32_sound_info.samplesPerCycle * win32_sound_info.sizeInCycles;
      win32_sound_info.sizeInBytes =
          (win32_sound_info.sizeInSamples * win32_sound_info.bitsPerSample) / 8;

      win32_Init_Xaudio2();
      WAVEFORMATEX waveformatex_struct{0};
      waveformatex_struct.wFormatTag = WAVE_FORMAT_PCM;
      waveformatex_struct.nChannels = 2;
      waveformatex_struct.nSamplesPerSec = win32_sound_info.samplesPerSecond;
      waveformatex_struct.nBlockAlign =
          (waveformatex_struct.nChannels * win32_sound_info.bitsPerSample) / 8;
      waveformatex_struct.nAvgBytesPerSec =
          waveformatex_struct.nSamplesPerSec * waveformatex_struct.nBlockAlign;
      waveformatex_struct.wBitsPerSample = win32_sound_info.bitsPerSample;
      waveformatex_struct.cbSize = 0;

      // filling out the xaudio2_buffer
      XAUDIO2_BUFFER xaudio2_buffer = {};
      xaudio2_buffer.AudioBytes = win32_sound_info.sizeInBytes;
      xaudio2_buffer.PlayBegin = 0;
      xaudio2_buffer.PlayLength = 0;
      xaudio2_buffer.LoopBegin = 0;
      xaudio2_buffer.LoopLength = 0;
      xaudio2_buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

      // create master voice
      IXAudio2MasteringVoice *master_voice;
      pXAudio2->CreateMasteringVoice(&master_voice);

      // create source voice
      pXAudio2->CreateSourceVoice(&source_voice, &waveformatex_struct);

      source_voice->Start(0);
      XAUDIO2_VOICE_STATE voice_state;
      int16_t *main_audio = (int16_t *)VirtualAlloc(
          NULL, 88200, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

#if HANDMADE_DEV
      LPVOID base_address = (LPVOID)terabytes((uint64_t)2);
#else
      LPVOID base_address = 0;
#endif
      game_memory_struct game_memory = {};
      game_memory.permanentSize = megabytes(64);
      game_memory.ramSize = gigabytes((uint64_t)1);

      uint64_t total_memory = game_memory.permanentSize + game_memory.ramSize;

      game_memory.permanent = VirtualAlloc(
          base_address, total_memory, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
      game_memory.ram =
          ((uint8_t *)game_memory.permanent + game_memory.permanentSize);

      // Performance Counting
      LARGE_INTEGER last_counter;
      QueryPerformanceCounter(&last_counter);
      uint64_t last_cpu_clock = __rdtsc();
      if (main_audio && game_memory.permanent && game_memory.ram) {

        game_input_struct input[2] = {};
        game_input_struct *new_input = &input[0];
        game_input_struct *old_input = &input[1];

        while (global_running) {

          game_controller_struct *keyboard_controller =
              &new_input->controllers[0];
          game_controller_struct zeroed = {};
          *keyboard_controller = zeroed;

          MSG msg;
          while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0) {
            if (msg.message == WM_QUIT)
              global_running = false;
            switch (msg.message) {
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP: {
              uint32_t keycode = (uint32_t)msg.wParam;
              bool was_down = (((msg.lParam & (1 << 30)) != 0));
              bool is_down = (((msg.lParam & (1 << 31)) == 0));
              if (was_down != is_down) {
                if (keycode == 'W') {
                } else if (keycode == 'A') {
                  // Process the INS key.
                } else if (keycode == 'S') {
                } else if (keycode == 'D') {
                } else if (keycode == 'Q') {
                  win32_Handle_Key_Input(&keyboard_controller->l_shoulder,
                                         is_down);
                } else if (keycode == 'E') {
                  win32_Handle_Key_Input(&keyboard_controller->r_shoulder,
                                         is_down);
                } else if (keycode == VK_LEFT) {
                  win32_Handle_Key_Input(&keyboard_controller->left, is_down);
                } else if (keycode == VK_RIGHT) {
                  win32_Handle_Key_Input(&keyboard_controller->right, is_down);
                } else if (keycode == VK_UP) {
                  win32_Handle_Key_Input(&keyboard_controller->up, is_down);
                } else if (keycode == VK_DOWN) {
                  win32_Handle_Key_Input(&keyboard_controller->down, is_down);
                } else if (keycode == VK_ESCAPE) {
                  global_running = false;
                }
              } else if (keycode == VK_SPACE) {
                OutputDebugString("SPACE PRESSED");
              }

              bool alt_key_was_down = ((msg.lParam & (1 << 29)) != 0);
              if (keycode == VK_F4 && alt_key_was_down) {
                global_running = false;
              }

            } break;

            default: {
              TranslateMessage(&msg);
              DispatchMessage(&msg);
            }
            }
          }

          // Gamepad input handling
          for (DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT;
               controllerIndex++) {
            XINPUT_STATE input_state{};
            ZeroMemory(&input_state, sizeof(XINPUT_STATE));
            if (XinputGetState(controllerIndex, &input_state) ==
                ERROR_SUCCESS) {
              // Controller is connected
              XINPUT_GAMEPAD *pad = &input_state.Gamepad;
              // do actions based on input received
              // NOTE: Maybe have to end up handling Deadzone
              // MSDN:
              // https: //
              //    learn.microsoft.com/en-us/windows/win32/xinput/getting-started-with-xinput#getting-controller-state

            } else {
              // Controller is not connected
            }
          }

          screen_dimensions_struct windowDimensions =
              win32_Get_Window_Dimensions(windHandle);

          win32_Resize_Dib_Sect(&global_offscreenBuffer, windowDimensions.width,
                                windowDimensions.height);

          game_offscreen_buffer_struct game_offscreen_buffer{};
          game_offscreen_buffer.memory = global_offscreenBuffer.memory;
          game_offscreen_buffer.height = global_offscreenBuffer.height;
          game_offscreen_buffer.width = global_offscreenBuffer.width;
          game_offscreen_buffer.bytesPerPixel =
              global_offscreenBuffer.bytesPerPixel;
          game_offscreen_buffer.pitch = global_offscreenBuffer.pitch;

          int16_t samples[88200];
          game_sound_buffer_struct game_sound_buffer = {};
          game_sound_buffer.samplesPerSecond =
              win32_sound_info.samplesPerSecond;
          game_sound_buffer.sampleCount = win32_sound_info.samplesPerSecond;
          game_sound_buffer.samples = samples;

          game_Update_Render(&game_memory, &game_offscreen_buffer,
                             &game_sound_buffer);

          if (source_voice->GetState(&voice_state),
              voice_state.BuffersQueued < XAUDIO2_MAX_QUEUED_BUFFERS) {
            win32_Fill_SoundBuffer(&game_sound_buffer, main_audio);

            xaudio2_buffer.AudioBytes = 88200;

            xaudio2_buffer.pAudioData = (byte *)main_audio;
            xaudio2_buffer.Flags = 0;

            source_voice->SubmitSourceBuffer(&xaudio2_buffer);
          }

          HDC deviceContext = GetDC(windHandle);

          win32_Display_Offscreen_Buffer(deviceContext, &global_offscreenBuffer,
                                         0, 0, windowDimensions.width,
                                         windowDimensions.height);
          ReleaseDC(windHandle, deviceContext);

          // calc performance stats
          uint64_t end_cpu_clock = __rdtsc();

          LARGE_INTEGER end_counter;
          QueryPerformanceCounter(&end_counter);

          uint64_t cycles_elapsed = end_cpu_clock - last_cpu_clock;
          int64_t counter_elapsed =
              end_counter.QuadPart - last_counter.QuadPart;

          double ms_per_frame =
              (((1000.0f * (double)counter_elapsed) / (double)frequency));
          double fps = ((double)frequency / (double)counter_elapsed);
          double mega_cycles_per_second =
              ((double)cycles_elapsed / (1000.0f * 1000.0f));

          char buffer[256];
          snprintf(buffer, sizeof(buffer), "%.02fms/f %.02ff/s %.02fmc/s\n",
                   ms_per_frame, fps, mega_cycles_per_second);
          // OutputDebugString(buffer);
          last_counter = end_counter;
          last_cpu_clock = end_cpu_clock;
        }
      }

    } else {
      // TODO: LOGGING
    }
  } else {
    // TODO: LOGGING
  }

  return 0;
}

LRESULT CALLBACK win32_Window_Proc(HWND hwnd, UINT msg, WPARAM wParam,
                                   LPARAM lParam) {
  LRESULT result = 0;
  switch (msg) {
  case WM_DESTROY: {
    global_running = false;
  } break;
  case WM_CLOSE: {
    global_running = false;
  } break;
  case WM_ACTIVATEAPP: {
    OutputDebugString("WM_ACTIVATEAPP");
  } break;

  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC deviceContext = BeginPaint(hwnd, &ps);

    int upperLeftX = ps.rcPaint.left;
    int upperLeftY = ps.rcPaint.top;

    screen_dimensions_struct windowDimensions =
        win32_Get_Window_Dimensions(hwnd);
    win32_Display_Offscreen_Buffer(
        deviceContext, &global_offscreenBuffer, upperLeftX, upperLeftY,
        windowDimensions.width, windowDimensions.height);
    EndPaint(hwnd, &ps);
  } break;
  default: {
    result = DefWindowProc(hwnd, msg, wParam, lParam);
  } break;
  }
  return result;
}

internal void win32_Resize_Dib_Sect(Win32offScreenBuf *buffer, int width,
                                    int height) {
  if (buffer->memory) {
    VirtualFree(buffer->memory, NULL, MEM_RELEASE);
  }

  buffer->width = width;
  buffer->height = height;

  buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
  buffer->info.bmiHeader.biWidth = buffer->width;
  buffer->info.bmiHeader.biHeight = -buffer->height;
  buffer->info.bmiHeader.biPlanes = 1;
  // NOTE: we are using 24bit color set, using 32bits for DWORD alignment,
  // to inc in 4 byte sets
  buffer->info.bmiHeader.biBitCount = 32;
  buffer->info.bmiHeader.biCompression = BI_RGB;

  buffer->bytesPerPixel = 4;
  int memorySize = buffer->bytesPerPixel * (buffer->width * buffer->height);

  buffer->memory = VirtualAlloc(NULL, memorySize, MEM_COMMIT, PAGE_READWRITE);

  buffer->pitch = buffer->width * buffer->bytesPerPixel;
}

internal void win32_Display_Offscreen_Buffer(HDC dc, Win32offScreenBuf *buffer,
                                             int x, int y, int wWidth,
                                             int wHeight) {
  // TODO: aspect ratio correction
  StretchDIBits(dc, 0, 0, wWidth, wHeight, 0, 0, buffer->width, buffer->height,
                buffer->memory, &buffer->info, DIB_RGB_COLORS, SRCCOPY);
}

internal int win32_Init_Xaudio2() {
  // INFO: load and create instance of xaduio2 sound engine
  HMODULE hXAudio2 = LoadLibrary("xaudio2_9.dll");
  if (!hXAudio2) {
    OutputDebugStringA("Failed to load xaudio2_9.dll");
    return 1;
  }

  XAUDIO2CREATE_def pXAudio2Create =
      (XAUDIO2CREATE_def)GetProcAddress(hXAudio2, "XAudio2Create");
  if (!pXAudio2Create) {
    OutputDebugStringA("Failed to get addr of XAudio2Create");
    FreeLibrary(hXAudio2);
    return 1;
  }

  HRESULT hr = pXAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
  if (FAILED(hr)) {
    OutputDebugStringA("Failed to use pXAudio2Create");
    FreeLibrary(hXAudio2);
    return 1;
  }

  OutputDebugStringA("Successfully created XAduio2 engine!");

  return 0;
}

internal void win32_Load_Libs() {
  HMODULE xinput_library = LoadLibraryA("xinput1_4.dll");
  if (!xinput_library)
    xinput_library = LoadLibraryA("xinput1_3.dll");
  if (xinput_library) {
    XinputGetState =
        (XINPUT_GET_STATE_def)GetProcAddress(xinput_library, "XInputGetState");
  } else {
    OutputDebugStringA("failed to load input mod");
  }
}

internal screen_dimensions_struct win32_Get_Window_Dimensions(HWND window) {
  screen_dimensions_struct result;
  RECT clientRect;
  GetClientRect(window, &clientRect);
  result.width = clientRect.right - clientRect.left;
  result.height = clientRect.bottom - clientRect.top;
  return result;
}

internal void win32_Fill_SoundBuffer(game_sound_buffer_struct *src_sound_buf,
                                     int16_t *dst_sound_buf) {

  int16_t *dst = dst_sound_buf;

  for (int bufferIndex = 0; bufferIndex < src_sound_buf->sampleCount;
       ++bufferIndex) {

    *dst++ = src_sound_buf->samples[bufferIndex];
  }
}

internal void debug_win32_FreeFile(void *memory) {
  if (memory) {
    VirtualFree(memory, 0, MEM_RELEASE);
  }
}

internal debug_win32_fileIO_struct debug_win32_ReadFile(char *filename) {
  debug_win32_fileIO_struct result = {};

  HANDLE file_handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ,
                                   NULL, OPEN_EXISTING, 0, 0);
  if (file_handle != INVALID_HANDLE_VALUE) {
    LARGE_INTEGER file_size64;
    if (GetFileSizeEx(file_handle, &file_size64)) {
      uint32_t file_size32 = safe_uint64_truncate(file_size64.QuadPart);

      result.data = VirtualAlloc(NULL, file_size32, MEM_COMMIT | MEM_RESERVE,
                                 PAGE_READWRITE);
      if (result.data) {
        DWORD bytes_read;
        if (ReadFile(file_handle, result.data, file_size32, &bytes_read,
                     NULL) &&
            bytes_read == file_size32) {

          result.size = bytes_read;

        } else {
          // TODO: LOGGING
          debug_win32_FreeFile(result.data);
          result.data = 0;
        }
      } else {
        // TODO: logging
      }
    } else {
      // TODO: logging
      CloseHandle(file_handle);
    }
  } else {
    // TODO: logging
  }

  return result;
}

internal bool debug_win32_WriteFile(char *filename, uint32_t size,
                                    void *memory) {
  bool result = false;

  HANDLE file_handle =
      CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
  if (file_handle != INVALID_HANDLE_VALUE) {

    DWORD bytes_written;
    if (WriteFile(file_handle, memory, size, &bytes_written, 0)) {
      result = (bytes_written == size);
    } else {
      // TODO: LOGGING
    }

    CloseHandle(file_handle);

  } else {
    // TODO: logging
  }

  return result;
}

internal void win32_Handle_Key_Input(game_button_state_struct *new_key_state,
                                     bool key_status) {
  new_key_state->ended_down = key_status;
  ++new_key_state->half_transition_count;
}
