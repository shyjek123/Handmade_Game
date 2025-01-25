#include <windows.h>
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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
      for (;;) {
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
    OutputDebugString("WM_SIZE");
  } break;
  case WM_DESTROY: {
    OutputDebugString("WM_DESTROY");
  } break;
  case WM_CLOSE: {

    OutputDebugString("WM_CLOSE");
  } break;
  case WM_ACTIVATEAPP: {
    OutputDebugString("WM_ACTIVATEAPP");
  } break;
  case WM_PAINT: {
    OutputDebugString("WM_PAINT");
    PAINTSTRUCT ps;
    HDC deviceContext = BeginPaint(hwnd, &ps);

    DWORD color = WHITENESS;

    int upperLeftX = ps.rcPaint.left;
    int upperLeftY = ps.rcPaint.top;
    int width = ps.rcPaint.right - upperLeftX;
    int height = ps.rcPaint.bottom - upperLeftY;

    PatBlt(deviceContext, upperLeftX, upperLeftY, width, height, color);
    EndPaint(hwnd, &ps);
  } break;
  default: {
    OutputDebugString("default");
    result = DefWindowProc(hwnd, msg, wParam, lParam);
  } break;
  }
  return result;
}
