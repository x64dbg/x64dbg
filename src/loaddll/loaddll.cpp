#include <windows.h>

wchar_t szLibraryPath[512];

static LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam,
                                LPARAM lParam) {
  static PAINTSTRUCT Paint = {0};
  if (Msg != WM_CREATE) {
    switch (Msg) {
    case WM_DESTROY:
      ExitProcess(0);
      break;
    case WM_PAINT:
      BeginPaint(hWnd, &Paint);
      EndPaint(hWnd, &Paint);
      break;
    case WM_CLOSE:
      DestroyWindow(hWnd);
      break;
    default:
      return DefWindowProcW(hWnd, Msg, wParam, lParam);
    }
  }
  return 0;
}

int WINAPI wWinMain(HINSTANCE hinst, HINSTANCE hprevinst, wchar_t *cmdline,
                    int cmdshow) {
  wchar_t szName[256];
  wsprintfW(szName, L"Local\\szLibraryName%X",
            (unsigned int)GetCurrentProcessId());
  HANDLE hMapFile = OpenFileMappingW(FILE_MAP_READ, false, szName);
  if (hMapFile) {
    const wchar_t *szLibraryPathMapping = (const wchar_t *)MapViewOfFile(
        hMapFile, FILE_MAP_READ, 0, 0, sizeof(szLibraryPath));
    if (szLibraryPathMapping) {
      lstrcpyW(szLibraryPath, szLibraryPathMapping);
      UnmapViewOfFile(szLibraryPathMapping);
    }
    CloseHandle(hMapFile);
  }
  if (szLibraryPath[0] && LoadLibraryW(szLibraryPath) != NULL) {
    WNDCLASSW wndCls = {0};
    wndCls.style = 0x2B;
    wndCls.lpfnWndProc = WndProc;
    wndCls.hInstance = GetModuleHandleW(NULL);
    wndCls.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndCls.lpszClassName = L"LoadDLLClass";
    RegisterClassW(&wndCls);
    HWND hWnd = CreateWindowExW(
        0, L"LoadDLLClass", L"x64dbg DLL Loader",
        WS_OVERLAPPED | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE |
            WS_SYSMENU | WS_THICKFRAME | WS_CAPTION,
        0x80000000, 0x80000000, 300, 100, 0, 0, wndCls.hInstance, 0);
    if (hWnd) {
      ShowWindow(hWnd, SW_SHOW);
      MSG msg = {0};
      while (GetMessage(&msg, hWnd, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
      }
    }
    return 1;
  }
  return 0;
}
