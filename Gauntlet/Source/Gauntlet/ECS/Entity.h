#pragma once

#include <stdint.h>
#include <bitset>
#include <vector>

static constexpr uint16_t s_MaxComponents = 32;
static constexpr uint64_t s_MaxEntities   = 1000;

using ComponentMask = std::bitset<s_MaxComponents>;

using Entity = std::uint32_t;
