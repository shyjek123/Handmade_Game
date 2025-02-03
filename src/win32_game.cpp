#include <cassert>
#include <stdint.h>
#include <windows.h>

#define internal static
#define local_persist static
#define global static

global bool Running;

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
  Win32resizeDibSect(&global_offscreenBuffer, 1280, 720);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = inst;
  wc.lpszClassName = CLASS_NAME;

  if (RegisterClass(&wc)) {
    HWND windHandle = CreateWindowEx(
        0, CLASS_NAME, "Handmade Hero", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL,
        inst, NULL);
    if (windHandle) {
      int x = 0, y = 0;
      Running = true;
      while (Running) {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0) {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }
        winDimensions windowDimensions = Win32getWindowDimensions(windHandle);
        renderColors(global_offscreenBuffer, x, y);
        HDC deviceContext = GetDC(windHandle);
        Win32DisplayOffScreenBuffer(deviceContext, &global_offscreenBuffer, 0,
                                    0, windowDimensions.width,
                                    windowDimensions.height);
        ReleaseDC(windHandle, deviceContext);
        ++x;
        y += 2;
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
  case WM_SIZE: {
  } break;
  case WM_DESTROY: {
    Running = false;
  } break;
  case WM_CLOSE: {
    Running = false;
  } break;
  case WM_ACTIVATEAPP: {
    OutputDebugString("WM_ACTIVATEAPP");
  } break;
  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC deviceContext = BeginPaint(hwnd, &ps);

    int upperLeftX = ps.rcPaint.left;
    int upperLeftY = ps.rcPaint.top;

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
  // NOTE: we are using 24bit color set, using 32bits for DWORD alignment, to
  // inc in 4 byte sets
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
      //      uint8_t Red = pixelY + windowX;
      *Pixel++ = (Green << 8) | Blue;
    }
    Row += buffer.pitch;
  }
}
