#include "assert.h"
#include "system.h"

Assert assert;

void Assert::operator()(LPCSTR message, LPCSTR title, Importance importance)
{
  MessageBox(winAPI.m_window_handle, message, title, MB_OK | importance);
}