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
        const auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - m_StartTime);
        return elapsed.count();
    }

  private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
};

}  // namespace Gauntlet