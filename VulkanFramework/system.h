#pragma once

#include <Windows.h>

extern class WinAPI
{
private:
  HWND m_window_handle;
  int m_initial_width = 1080;
  int m_initial_height = 720;
  int m_width = m_initial_width;
  int m_height = m_initial_height;

  RECT screen_client;
  RECT screen_window;

  HDC m_hdc;
  HGLRC m_hglrc;

public:
  int Width() const { return m_width; }
  int Height() const { return m_height; }
  const HWND& WindowHandle() const { return m_window_handle; }

  friend void UpdateClientRect(const HWND& hwnd);
  friend LRESULT CALLBACK MainWindowCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  friend HWND InitializeWindow(WNDCLASS& wndClass, const HINSTANCE& hInst);
  friend int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int);
  friend void Update();
} winAPI;

extern class Engine
{
private:
  bool m_quit = false;
  bool m_restart = false;

public:
  void Quit() { m_quit = true; m_restart = true; }
  void Restart() { m_restart = true; }

  friend int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
} engine;