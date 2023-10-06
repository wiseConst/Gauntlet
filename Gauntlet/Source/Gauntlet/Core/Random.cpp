#include "GauntletPCH.h"
#include "Random.h"

namespace Gauntlet
{
std::mt19937_64 Random::s_Engine(Random::s_RandomDevice());

float Random::GetInRange(float begin, float end)
{
    std::uniform_real_distribution<float> s_UniformDistribution(begin, end);

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