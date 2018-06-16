#pragma once

#include <chrono>

extern class Time
{
private:
  std::chrono::time_point<std::chrono::steady_clock> m_begin_time;
  std::chrono::time_point<std::chrono::steady_clock> m_prev_time;
  float m_delta_time = 0;

  void Begin();
  void Update();

public:
  float dt() const { return m_delta_time; }
  
  friend void Begin();
  friend void Update();
} timer;