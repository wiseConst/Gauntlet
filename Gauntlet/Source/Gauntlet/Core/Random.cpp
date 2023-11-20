#include "GauntletPCH.h"
#include "Random.h"

namespace Gauntlet
{
std::default_random_engine Random::s_Engine(0);
static std::uniform_real_distribution<float> s_UniformDistribution(0.0f, 1.0f);

float Random::GetInRange0To1()
{
    return s_UniformDistribution(s_Engine);
}

float Random::GetNoise(float x, float y)
{
    static bool bIsInitialized = false;
    if (!bIsInitialized)
    {
        s_Noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        s_Noise.SetSeed(1337);

        s_Noise.SetFrequency(0.01f);
        s_Noise.SetFractalType(FastNoiseLite::FractalType_FBm);
        s_Noise.SetFractalLacunarity(2.0f);
        s_Noise.SetFractalGain(0.5f);

        bIsInitialized = true;
    }

    return s_Noise.GetNoise(x, y);
}

}  // namespace Gauntlet