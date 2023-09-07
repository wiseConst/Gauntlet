#include "GauntletPCH.h"
#include "UUID.h"

#include <random>

namespace Gauntlet
{
static std::random_device s_RandomDevice;                              // Seeding random number generator
static std::mt19937_64 s_Engine(s_RandomDevice());                     // Filling engine
static std::uniform_int_distribution<uint64_t> s_UniformDistribution;  // Getting

UUID::UUID() : m_UUID(s_UniformDistribution(s_Engine)) {}

UUID::UUID(uint64_t uuid) : m_UUID(uuid) {}

}  // namespace Gauntlet