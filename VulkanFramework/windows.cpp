#include <Windows.h>
#include <chrono>

#ifndef NDEBUG
#define CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#endif

#include "system.h"
#include "time.h"

//todo:
// Initialize ~ Terminate
// Input
// Vulkan lib

void UpdateClientRect(const HWND& hwnd)
{
  POINT p;
  p.x = winAPI.screen_client.left;
  p.y = winAPI.screen_client.top;
  ClientToScreen(hwnd, &p);
  winAPI.screen_client.left = p.x;
  winAPI.screen_client.top = p.y;
  p.x = winAPI.screen_client.right;
  p.y = winAPI.screen_client.bottom;
  ClientToScreen(hwnd, &p);
  winAPI.screen_client.right = p.x;
  winAPI.screen_client.bottom = p.y;

  winAPI.m_width = winAPI.screen_client.right - winAPI.screen_client.left;
  winAPI.m_height = winAPI.screen_client.bottom - winAPI.screen_client.top;
}

LRESULT CALLBACK MainWindowCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
  case WM_SETCURSOR:
    static HCURSOR normal_cursor = LoadCursor(nullptr, IDC_ARROW);
    SetCursor(normal_cursor);
    break;

  case WM_SIZE:
    GetClientRect(hwnd, &winAPI.screen_client);
    GetWindowRect(hwnd, &winAPI.screen_window);
    UpdateClientRect(hwnd);
    //graphics.ResizeScreen();
    break;

  case WM_MOVE:
    GetClientRect(hwnd, &winAPI.screen_client);
    GetWindowRect(hwnd, &winAPI.screen_window);
    UpdateClientRect(hwnd);
    break;

  case WM_CLOSE:
  case WM_DESTROY:
    engine.Quit();
    //wglMakeCurrent(winAPI.m_hdc, NULL);
    //wglDeleteContext(winAPI.m_hglrc);
    ReleaseDC(hwnd, winAPI.m_hdc);
    break;

  default:
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
  }

  return 0;
}

HWND InitializeWindow(WNDCLASS& wndClass, const HINSTANCE& hInst)
{
  wndClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW; //window class style
  wndClass.lpfnWndProc = MainWindowCallback;
  wndClass.hInstance = hInst;
  wndClass.lpszClassName = "Cab Framework";

  if (RegisterClass(&wndClass))
    return CreateWindowEx(0, wndClass.lpszClassName, "Vulkan Framework", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, winAPI.m_initial_width, winAPI.m_initial_height, 0, 0, wndClass.hInstance, 0);

  return nullptr;
}

bool Initialize()
{
  srand((unsigned)time(NULL));

#ifndef NDEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); //memory leak check
#endif

  //console.Initialize();
  //if (!graphics.Initialize()) return false;
  //models.Initialize();

  return true;
}

void Begin()
{
  //level->Begin();
  //graphics.Begin();
  timer.Begin();
  //input.Begin();
}

void Update()
{
  //level->Update();
  //graphics.Update();
  //MyImGui::Implementation();
  SwapBuffers(winAPI.m_hdc);
  //input.Update();
  timer.Update();
}

void End()
{
  //graphics.End();
  //level->End();
}

void Terminate()
{
  //graphics.Terminate();
  //models.Terminate();
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
  WNDCLASS windowClass = {};
  winAPI.m_window_handle = InitializeWindow(windowClass, hInstance);
  if (!winAPI.m_window_handle) return 0;

  if (!Initialize()) return 0;

  MSG msg;
  while (!engine.m_quit)
  {
    engine.m_restart = false;
    Begin();
    while (!engine.m_restart)
    {
      while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
      Update();
    }
    End();
  }
  Terminate();

  return 0;
}