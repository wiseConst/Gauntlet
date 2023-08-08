#pragma once

#include <Gauntlet/Core/Core.h>

namespace Gauntlet
{

class Timestep final
{
  public:
    Timestep(float InTimeMilliSeconds = 0.0f) : m_MilliSeconds(InTimeMilliSeconds) {}

    FORCEINLINE operator float() const { return m_MilliSeconds; }

    FORCEINLINE float GetSeconds() const { return m_MilliSeconds / 1000.0f; }
    FORCEINLINE float GetMilliseconds() const { return m_MilliSeconds; }

  private:
    float m_MilliSeconds;
};

}  // namespace Gauntlet