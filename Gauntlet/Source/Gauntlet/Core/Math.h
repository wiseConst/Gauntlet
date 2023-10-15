#pragma once

#pragma warning(disable : 4005)

#include "Core.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Gauntlet
{

namespace Math
{
FORCEINLINE float Lerp(float a, float b, float f)
{
    return a + f * (b - a);
}
}  // namespace Math

}  // namespace Gauntlet