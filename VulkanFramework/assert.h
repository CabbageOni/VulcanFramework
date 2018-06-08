#pragma once

#include <windows.h>

extern class Assert
{
public:
  enum Importance : long { Info = 0x0L, Warn = MB_ICONWARNING, Error = MB_ICONERROR };

  void operator()(LPCSTR message, LPCSTR title = "Vulkan Framework", Importance importance = Info);
}assert;