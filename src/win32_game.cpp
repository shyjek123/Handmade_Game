#include <windows.h>

#define internal static
#define local_persist static
#define global static

global bool Running;

global BITMAPINFO bitmapInfo;
global void *bitmapmem;
global HBITMAP bmpHandle;
global HDC BitmapDeviceContext;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
internal void Win32resizeDibSect(int width, int height);
internal void Win32updateWind(HDC dc, int x, int y, int width, int height);

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
      MSG msg = {};
      Running = true;
      while (Running) {
        if (GetMessage(&msg, NULL, 0, 0) > 0) {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        } else {
          break;
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
    Win32updateWind(deviceContext, upperLeftX, upperLeftY, width, height);
    EndPaint(hwnd, &ps);
  } break;
  default: {
    result = DefWindowProc(hwnd, msg, wParam, lParam);
  } break;
  }
  return result;
}

internal void Win32resizeDibSect(int width, int height) {
  if (bmpHandle)
    DeleteObject(bmpHandle);
  if (!BitmapDeviceContext)
    BitmapDeviceContext = CreateCompatibleDC(0);

  bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
  bitmapInfo.bmiHeader.biWidth = width;
  bitmapInfo.bmiHeader.biHeight = height;
  bitmapInfo.bmiHeader.biPlanes = 1;
  bitmapInfo.bmiHeader.biBitCount = 32;
  bitmapInfo.bmiHeader.biCompression = BI_RGB;
  bmpHandle = CreateDIBSection(BitmapDeviceContext, &bitmapInfo, DIB_RGB_COLORS,
                               &bitmapmem, NULL, NULL);
}

internal void Win32updateWind(HDC dc, int x, int y, int width, int height) {
  StretchDIBits(dc, x, y, width, height, x, y, width, height, bitmapmem,
                &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}
