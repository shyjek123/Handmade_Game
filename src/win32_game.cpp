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

internal void Fill_Audio_Buffer(byte *main_audio);

internal void win32_Load_Libs();

internal void win32_Display_Offscreen_Buffer(HDC dc, Win32offScreenBuf *buffer,
                                             int x, int y, int width,
                                             int height);

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
      double x = 0, y = 0;

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
      waveformatex_struct.nChannels = 1;
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
      XAUDIO2_VOICE_STATE state;
      int16_t *main_audio = (int16_t *)VirtualAlloc(
          NULL, win32_sound_info.sizeInBytes, MEM_COMMIT, PAGE_READWRITE);

      // Performance Counting
      LARGE_INTEGER last_counter;
      QueryPerformanceCounter(&last_counter);
      uint64_t last_cpu_clock = __rdtsc();

      while (global_running) {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0) {
          if (msg.message == WM_QUIT)
            global_running = false;

          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }

        // Gamepad input handling
        for (DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT;
             controllerIndex++) {
          XINPUT_STATE state{};
          ZeroMemory(&state, sizeof(XINPUT_STATE));
          if (XinputGetState(controllerIndex, &state) == ERROR_SUCCESS) {
            // Controller is connected
            XINPUT_GAMEPAD *pad = &state.Gamepad;
            // do actions based on input received
            // NOTE: Maybe have to end up handling Deadzone
            // MSDN:
            // https: //
            //    learn.microsoft.com/en-us/windows/win32/xinput/getting-started-with-xinput#getting-controller-state

          } else {
            // Controller is not connected
          }
        }

        x += 0.5;

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
        game_sound_buffer.samplesPerSecond = win32_sound_info.samplesPerSecond;
        game_sound_buffer.sampleCount = win32_sound_info.samplesPerSecond;
        game_sound_buffer.samples = samples;

        game_Update_Render(&game_offscreen_buffer, x, y, &game_sound_buffer);

        if (source_voice->GetState(&state),
            state.BuffersQueued < XAUDIO2_MAX_QUEUED_BUFFERS) {
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
        int64_t counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;

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
  case WM_SYSKEYDOWN:
  case WM_SYSKEYUP:
  case WM_KEYDOWN:
  case WM_KEYUP: {
    uint32_t keycode = wParam;
    bool was_down = (((lParam & (1 << 30)) != 0));
    bool is_down = (((lParam & (1 << 31)) == 0));
    if (was_down != is_down) {
      if (keycode == 'W') {

      } else if (keycode == 'A') {
        // Process the INS key.
      } else if (keycode == 'S') {
      } else if (keycode == 'D') {
      } else if (keycode == 'Q') {
      } else if (keycode == 'E') {

      } else if (keycode == VK_LEFT) {
        // Process the LEFT ARROW key.

      } else if (keycode == VK_RIGHT) {
        // Process the RIGHT ARROW key.

      } else if (keycode == VK_UP) {
        // Process the UP ARROW key.

      } else if (keycode == VK_DOWN) {
        // Process the DOWN ARROW key.

      } else if (keycode == VK_ESCAPE) {
        OutputDebugString("ESCAPE: ");
        if (is_down)
          OutputDebugString("is_down ");
        if (was_down)
          OutputDebugString("was_down ");
        OutputDebugString("\n");
      }
    } else if (keycode == VK_SPACE) {
      OutputDebugString("SPACE PRESSED");
    }

    bool alt_key_was_down = ((lParam & (1 << 29)) != 0);
    if (keycode == VK_F4 && alt_key_was_down) {
      global_running = false;
    }

  } break;

  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC deviceContext = BeginPaint(hwnd, &ps);

    int upperLeftX = ps.rcPaint.left;
    int upperLeftY = ps.rcPaint.top;
    // casey does not use
    // int width = ps.rcPaint.right - ps.rcPaint.left;
    // int height = ps.rcPaint.bottom - ps.rcPaint.top;

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

  for (int idx = 0; idx < src_sound_buf->sampleCount; idx++) {
    *dst_sound_buf++ = src_sound_buf->samples[idx];
  }
}
