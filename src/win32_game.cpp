#include <cstdint>
#include <windows.h>

#define internal static
#define local_persist static
#define global static

global bool Running;

global BITMAPINFO bitmapInfo;
global void *bitmapMemory;
global int bitmapWidth;
global int bitmapHeight;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
internal void renderColors(int width, int height, int windowX, int winddowY,
                           int bytesPerPixel);
internal void Win32resizeDibSect(int width, int height);
internal void Win32updateWind(HDC dc, RECT *WindowRect, int x, int y, int width,
                              int height);

int CALLBACK WinMain(HINSTANCE inst, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
                     int nCmdShow) {
  const char CLASS_NAME[] = "HandmadeWndClass";

  WNDCLASS wc = {};
  wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
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
      MSG msg = {};
      Running = true;
      while (Running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0) {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }

        RECT clientRect;
        GetClientRect(windHandle, &clientRect);
        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;
        renderColors(width, height, x, y, 4);
        HDC deviceContext = GetDC(windHandle);
        Win32updateWind(deviceContext, &clientRect, x, y, width, height);
        ReleaseDC(windHandle, deviceContext);
        x++;
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
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;
    Win32resizeDibSect(width, height);
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
    int width = ps.rcPaint.right - upperLeftX;
    int height = ps.rcPaint.bottom - upperLeftY;

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);

    Win32updateWind(deviceContext, &clientRect, upperLeftX, upperLeftY, width,
                    height);
    EndPaint(hwnd, &ps);
  } break;
  default: {
    result = DefWindowProc(hwnd, msg, wParam, lParam);
  } break;
  }
  return result;
}

internal void Win32resizeDibSect(int width, int height) {
  if (bitmapMemory) {
    VirtualFree(bitmapMemory, NULL, MEM_RELEASE);
  }

  bitmapWidth = width;
  bitmapHeight = height;

  bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
  bitmapInfo.bmiHeader.biWidth = bitmapWidth;
  bitmapInfo.bmiHeader.biHeight = -bitmapHeight;
  bitmapInfo.bmiHeader.biPlanes = 1;
  // NOTE: we are using 24bit color set, using 32 for DWORD alignment, to inc in
  // 4 byte sets
  bitmapInfo.bmiHeader.biBitCount = 32;
  bitmapInfo.bmiHeader.biCompression = BI_RGB;

  int bytesPerPixel = 4;
  int bitmapMemorySize = bytesPerPixel * (width * height);

  bitmapMemory =
      VirtualAlloc(NULL, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32updateWind(HDC dc, RECT *WindowRect, int x, int y, int width,
                              int height) {

  int windowWidth = WindowRect->right - WindowRect->left;
  int windowHeight = WindowRect->bottom - WindowRect->top;
  StretchDIBits(dc, 0, 0, bitmapWidth, bitmapHeight, 0, 0, windowWidth,
                windowHeight, bitmapMemory, &bitmapInfo, DIB_RGB_COLORS,
                SRCCOPY);
}

internal void renderColors(int width, int height, int windowX, int windowY,
                           int bytesPerPixel) {

  int Pitch = width * bytesPerPixel;
  uint8_t *Row = (uint8_t *)bitmapMemory;
  uint32_t *Pixel;
  for (int pixelY = 0; pixelY < height; pixelY++) {
    Pixel = (uint32_t *)Row;
    for (int pixelX = 0; pixelX < width; pixelX++) {
      uint8_t Green = pixelX + windowX;
      uint8_t Blue = pixelY + windowY;
      uint8_t Red = pixelY + windowX;
      *Pixel++ = (Green << 8) | Blue | (Red << 16);
    }
    Row += Pitch;
  }
}
