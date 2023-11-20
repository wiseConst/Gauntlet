#pragma once

#include "Core.h"
#include <random>
#include "FastNoiseLite/Cpp/FastNoiseLite.h"

namespace Gauntlet
{

class Random final : private Uncopyable, private Unmovable
{
  public:
    static float GetInRange0To1();
    static float GetNoise(float x, float y);
    FORCEINLINE static void SetNoiseSeed(int32_t seed) { s_Noise.SetSeed(seed); }

  private:
    static std::default_random_engine s_Engine;
    static inline FastNoiseLite s_Noise;
};

}  // namespace Gauntlet