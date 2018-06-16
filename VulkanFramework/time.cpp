#include <chrono>

#include "time.h"

Time timer;

void Time::Begin()
{
  m_begin_time = std::chrono::high_resolution_clock::now();
  m_prev_time = m_begin_time;
}

void Time::Update()
{
  std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::high_resolution_clock::now();
  m_delta_time = std::chrono::duration_cast<std::chrono::microseconds>(now - m_prev_time).count() / 1000000.0f;
  m_prev_time = now;
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