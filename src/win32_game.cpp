#include <cassert>
#include <cstdio>
#include <math.h>
#include <stdint.h>
#include <windows.h>
#include <xaudio2.h>
#include <xinput.h>
#define internal static
#define local_persist static
#define global static
typedef int32_t bool32;

// INFO: AUDIO VARS
global IXAudio2 *pXAudio2{0};
global IXAudio2SourceVoice *source_voice{0};

constexpr WORD BITSPERSSAMPLE = 16;    // 16 bits per sample.
constexpr DWORD SAMPLESPERSEC = 44100; // 44,100 samples per second.
// 220 cycles per second (frequency of the audible tone).
constexpr double CYCLESPERSEC = 174.0;
constexpr double VOLUME = 0.05;
constexpr DWORD FADEDURATION = 500;
constexpr WORD AUDIOBUFFERSIZEINCYCLES = 10; // 10 cycles per audio buffer.
constexpr double PI = 3.14159265358979323846;

// Calculated constants.
constexpr DWORD SAMPLESPERCYCLE =
    (DWORD)(SAMPLESPERSEC / CYCLESPERSEC); // 200 samples per cycle.
constexpr DWORD AUDIOBUFFERSIZEINSAMPLES =
    SAMPLESPERCYCLE * AUDIOBUFFERSIZEINCYCLES; // 2,000 samples per buffer.
constexpr UINT32 AUDIOBUFFERSIZEINBYTES =
    AUDIOBUFFERSIZEINSAMPLES * BITSPERSSAMPLE / 8; // 4,000 bytes per buffer.
// end audio vars

global bool global_running;
struct Win32offScreenBuf {
  BITMAPINFO info;
  void *memory;
  int width;
  int height;
  int bytesPerPixel;
  int pitch;
};

global Win32offScreenBuf global_offscreenBuffer;

struct winDimensions {
  int width;
  int height;
};

internal winDimensions Win32getWindowDimensions(HWND window) {
  winDimensions result;
  RECT clientRect;
  GetClientRect(window, &clientRect);
  result.width = clientRect.right - clientRect.left;
  result.height = clientRect.bottom - clientRect.top;
  return result;
}
typedef HRESULT(WINAPI *XAUDIO2CREATE_def)(IXAudio2 **ppXAudio2, UINT32 Flags,
                                           XAUDIO2_PROCESSOR XAudio2Processor);
typedef DWORD(WINAPI *XINPUT_GET_STATE_def)(DWORD dwUserIndex,
                                            XINPUT_STATE *pState);
global XINPUT_GET_STATE_def XinputGetState;

internal int init_xaudio2() {
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

internal void LoadNeededWin32Libs() {
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

VOID fill_audio_buffer(byte *main_audio) {
  double phase{};
  uint32_t bufferidx{};

  while (bufferidx < AUDIOBUFFERSIZEINBYTES) {
    phase += (2.0 * PI) / SAMPLESPERCYCLE;
    int16_t sample = (int16_t)(sin(phase) * INT16_MAX * VOLUME);

    // writing values in little endian
    main_audio[bufferidx++] = (byte)sample;
    main_audio[bufferidx++] = (byte)(sample >> 8);
  }
}

DWORD WINAPI AudioThreadProc(LPVOID lParam) {
  // TODO: Audio Test
  //   1. populate a WAVEFORMATEX structure and an XAUDIO2_BUFFER
  //   structure
  //   2. create source and master voice
  //   3. submit audio buffer to source voice
  //   4. call start function

  return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

internal void renderColors(Win32offScreenBuf buffer, int WindowX, int WindowY);

internal void Win32resizeDibSect(Win32offScreenBuf *buffer, int width,
                                 int height);

internal void Win32DisplayOffScreenBuffer(HDC dc, Win32offScreenBuf *buffer,
                                          int x, int y, int width, int height);

int CALLBACK WinMain(HINSTANCE inst, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
                     int nCmdShow) {
  const char CLASS_NAME[] = "HandmadeWndClass";

  WNDCLASS wc = {};

  wc.style = CS_VREDRAW | CS_HREDRAW;
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = inst;
  wc.lpszClassName = CLASS_NAME;

  if (RegisterClass(&wc)) {
    HWND windHandle = CreateWindowEx(
        0, wc.lpszClassName, "Handmade Hero", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, inst,
        0);
    if (windHandle) {
      global_running = true;
      double x = 0, y = 0;

      winDimensions dimensions = Win32getWindowDimensions(windHandle);
      Win32resizeDibSect(&global_offscreenBuffer, dimensions.width,
                         dimensions.height);

      LoadNeededWin32Libs();

      // INFO: audio stuff
      init_xaudio2();
      // create master voice
      IXAudio2MasteringVoice *master_voice;
      pXAudio2->CreateMasteringVoice(&master_voice);

      //  define the audio settings/format
      WAVEFORMATEX waveformatex_struct{0};
      waveformatex_struct.wFormatTag = WAVE_FORMAT_PCM;
      waveformatex_struct.nChannels = 2;
      waveformatex_struct.nSamplesPerSec = SAMPLESPERSEC;
      waveformatex_struct.nBlockAlign =
          (waveformatex_struct.nChannels * BITSPERSSAMPLE) / 8;
      waveformatex_struct.nAvgBytesPerSec =
          waveformatex_struct.nSamplesPerSec * waveformatex_struct.nBlockAlign;
      waveformatex_struct.wBitsPerSample = BITSPERSSAMPLE;
      waveformatex_struct.cbSize = 0;

      // filling out the xaudio2_buffer
      XAUDIO2_BUFFER xaudio2_buffer = {};
      xaudio2_buffer.Flags = XAUDIO2_END_OF_STREAM;
      xaudio2_buffer.AudioBytes = AUDIOBUFFERSIZEINBYTES;
      xaudio2_buffer.PlayBegin = 0;
      xaudio2_buffer.PlayLength = 0;
      xaudio2_buffer.LoopBegin = 0;
      xaudio2_buffer.LoopLength = 0;
      xaudio2_buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

      // create source voice
      pXAudio2->CreateSourceVoice(&source_voice, &waveformatex_struct);

      source_voice->Start(0);

      XAUDIO2_VOICE_STATE state;

      while (global_running) {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0) {
          if (msg.message == WM_QUIT)
            global_running = false;

          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }
        if (source_voice->GetState(&state),
            state.BuffersQueued < XAUDIO2_MAX_QUEUED_BUFFERS) {
          // create/fill main audio buffer
          byte main_audio[AUDIOBUFFERSIZEINBYTES] = {};
          fill_audio_buffer(main_audio);

          xaudio2_buffer.AudioBytes = AUDIOBUFFERSIZEINBYTES;
          xaudio2_buffer.pAudioData = main_audio;
          xaudio2_buffer.Flags = 0;
          source_voice->SubmitSourceBuffer(&xaudio2_buffer);
        }

        winDimensions windowDimensions = Win32getWindowDimensions(windHandle);

        Win32resizeDibSect(&global_offscreenBuffer, windowDimensions.width,
                           windowDimensions.height);
        // Drawing to Window
        renderColors(global_offscreenBuffer, x, y);
        HDC deviceContext = GetDC(windHandle);

        Win32DisplayOffScreenBuffer(deviceContext, &global_offscreenBuffer, 0,
                                    0, windowDimensions.width,
                                    windowDimensions.height);
        ReleaseDC(windHandle, deviceContext);

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
      }

    } else {
      // TODO: LOGGING
    }
  } else {
    // TODO: LOGGING
  }

  return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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

    winDimensions windowDimensions = Win32getWindowDimensions(hwnd);
    Win32DisplayOffScreenBuffer(deviceContext, &global_offscreenBuffer,
                                upperLeftX, upperLeftY, windowDimensions.width,
                                windowDimensions.height);
    EndPaint(hwnd, &ps);
  } break;
  default: {
    result = DefWindowProc(hwnd, msg, wParam, lParam);
  } break;
  }
  return result;
}

internal void Win32resizeDibSect(Win32offScreenBuf *buffer, int width,
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

internal void Win32DisplayOffScreenBuffer(HDC dc, Win32offScreenBuf *buffer,
                                          int x, int y, int wWidth,
                                          int wHeight) {
  // TODO: aspect ratio correction
  StretchDIBits(dc, 0, 0, wWidth, wHeight, 0, 0, buffer->width, buffer->height,
                buffer->memory, &buffer->info, DIB_RGB_COLORS, SRCCOPY);
}

internal void renderColors(Win32offScreenBuf buffer, int windowX, int windowY) {
  uint8_t *Row = (uint8_t *)buffer.memory;
  uint32_t *Pixel;
  for (int pixelY = 0; pixelY < buffer.height; ++pixelY) {
    Pixel = (uint32_t *)Row;
    for (int pixelX = 0; pixelX < buffer.width; ++pixelX) {
      uint8_t Green = pixelX + windowX;
      uint8_t Blue = pixelY + windowY;
      *Pixel++ = (Green << 8) | Blue;
    }
    Row += buffer.pitch;
  }
}
