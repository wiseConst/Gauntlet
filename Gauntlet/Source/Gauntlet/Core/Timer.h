#pragma once

#include "Gauntlet/Core/Core.h"
#include <chrono>

namespace Gauntlet
{
class Timer final
{
  public:
    Timer() : m_StartTime(std::chrono::high_resolution_clock::now()) {}
    ~Timer() = default;

    static double Now();

    FORCEINLINE double GetElapsedMilliseconds() const
    {
        const std::chrono::duration<double, std::milli> elapsed = std::chrono::high_resolution_clock::now() - m_StartTime;
        return elapsed.count();
    }

  private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
};
}  // namespace Gauntlet