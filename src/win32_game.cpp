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
internal void resizeDibSect(int width, int height);
internal void updateWind(HDC dc, int x, int y, int width, int height);

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
    RECT rect;
    GetClientRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    resizeDibSect(width, height);
    OutputDebugString("WM_SIZE");
  } break;
  case WM_DESTROY: {
    Running = false;
    OutputDebugString("WM_DESTROY");
  } break;
  case WM_CLOSE: {
    Running = false;
    OutputDebugString("WM_CLOSE");
  } break;
  case WM_ACTIVATEAPP: {
    OutputDebugString("WM_ACTIVATEAPP");
  } break;
  case WM_PAINT: {
    OutputDebugString("WM_PAINT");
    PAINTSTRUCT ps;
    HDC deviceContext = BeginPaint(hwnd, &ps);

    int upperLeftX = ps.rcPaint.left;
    int upperLeftY = ps.rcPaint.top;
    int width = ps.rcPaint.right - upperLeftX;
    int height = ps.rcPaint.bottom - upperLeftY;
    // TODO: update the window stretch the bits
    updateWind(deviceContext, upperLeftX, upperLeftY, width, height);
    EndPaint(hwnd, &ps);
  } break;
  default: {
    OutputDebugString("default");
    result = DefWindowProc(hwnd, msg, wParam, lParam);
  } break;
  }
  return result;
}

internal void resizeDibSect(int width, int height) {
  if (bmpHandle) {
    DeleteObject(bmpHandle);
  }
  if (!BitmapDeviceContext) {
    BitmapDeviceContext = CreateCompatibleDC(0);
  }

  bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
  bitmapInfo.bmiHeader.biWidth = width;
  bitmapInfo.bmiHeader.biHeight = height;
  bitmapInfo.bmiHeader.biPlanes = 1;
  bitmapInfo.bmiHeader.biPlanes = 32;
  bitmapInfo.bmiHeader.biCompression = BI_RGB;

  bmpHandle = CreateDIBSection(BitmapDeviceContext, &bitmapInfo, DIB_RGB_COLORS,
                               &bitmapmem, NULL, NULL);
}

internal void updateWind(HDC dc, int x, int y, int width, int height) {
  const void *img;
  const BITMAPINFO *bitmapInfo;
  StretchDIBits(dc, x, y, width, height, x, y, width, height, img, bitmapInfo,
                DIB_RGB_COLORS, SRCCOPY);
}
