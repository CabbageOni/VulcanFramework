#include <chrono>
#include <Windows.h>
#include <string>

#include "time.h"

Timer timer;

void Timer::Begin()
{
  m_begin_time = std::chrono::high_resolution_clock::now();
  m_prev_time = m_begin_time;
}

void Timer::Update()
{
  auto& get_now = std::chrono::high_resolution_clock::now;
  std::chrono::time_point<std::chrono::steady_clock> now = get_now();
  m_delta_time = std::chrono::duration_cast<std::chrono::microseconds>(now - m_prev_time).count() / 1000000.0f;
  m_prev_time = now;

  // frame lock
  int64_t miliseconds_left_to_sleep = 16 - std::chrono::duration_cast<std::chrono::milliseconds>(get_now() - m_prev_time).count();
  while(miliseconds_left_to_sleep > 0)
  {
    Sleep(miliseconds_left_to_sleep);
    miliseconds_left_to_sleep = 16 - std::chrono::duration_cast<std::chrono::milliseconds>(get_now() - m_prev_time).count();
  }
}

//#ifndef NDEBUG
//void Time::PerformanceCheckFrom()
//{
//  performanceCheck = std::chrono::high_resolution_clock::now();
//}
//
//void Time::PerformanceCheckTo()
//{
//  using namespace std::chrono;
//
//  performanceCheckAverage += duration_cast<nanoseconds>(high_resolution_clock::now() - performanceCheck).count();
//  ++performanceCheckFrameCount;
//}
//#endif