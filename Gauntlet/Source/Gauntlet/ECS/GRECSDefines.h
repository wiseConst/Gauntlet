#pragma once

#include <bitset>

namespace Gauntlet
{

namespace GRECS
{
static constexpr uint16_t s_MaxComponents = 32;
static constexpr uint64_t s_MaxEntities   = 1000;  // TODO: How to extend?

using ComponentMask = std::bitset<s_MaxComponents>;

using Entity = std::uint32_t;
}  // namespace GRECS

}  // namespace Gauntlet