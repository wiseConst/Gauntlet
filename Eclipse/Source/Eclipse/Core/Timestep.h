#pragma once

#include <Eclipse/Core/Core.h>
#include <chrono>

namespace Eclipse
{
class Timestep final
{
  public:
    Timestep() : m_LastFrameTime(std::chrono::high_resolution_clock::now()), m_DeltaTime() {}
    ~Timestep() = default;

    FORCEINLINE void Update()
    {
        auto CurrentTime = std::chrono::high_resolution_clock::now();
        m_DeltaTime = CurrentTime - m_LastFrameTime;
        m_LastFrameTime = CurrentTime;
    }

    FORCEINLINE float GetSeconds() const { return m_DeltaTime.count() / 1000000000.0f; }
    FORCEINLINE float GetMilliseconds() const { return m_DeltaTime.count() / 1000000.0f; }
    FORCEINLINE float GetMicroseconds() const { return m_DeltaTime.count() / 1000.0f; }
    FORCEINLINE float GetDeltaTime() const { return GetMilliseconds(); }

    FORCEINLINE operator float() const { return GetMilliseconds(); }

  private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_LastFrameTime;
    std::chrono::duration<long long, std::nano> m_DeltaTime;
};
}  // namespace Eclipse