#pragma once

#include "Core.h"
#include <random>
#include "FastNoiseLite/Cpp/FastNoiseLite.h"

namespace Gauntlet
{

class Random final : private Uncopyable, private Unmovable
{
  public:
    static float GetInRange(float begin, float end);
    static float GetNoise(float x, float y);
    FORCEINLINE static void SetNoiseSeed(int32_t seed) { s_Noise.SetSeed(seed); }

  private:
    static inline std::random_device s_RandomDevice;  // Seeding random number generator
    static std::mt19937_64 s_Engine;                  // Filling engine
    static inline FastNoiseLite s_Noise;
};

}  // namespace Gauntlet