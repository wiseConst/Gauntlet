#pragma once

#pragma warning(disable : 4005)

#include "Core.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Gauntlet
{

namespace Math
{

FORCEINLINE constexpr float Lerp(float a, float b, float t)
{
    // return a + t * (b - a);
    return a * (1 - t) + b * t;
}

}  // namespace Math

}  // namespace Gauntlet