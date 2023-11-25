#include "GauntletPCH.h"
#include "Timer.h"

#include <GLFW/glfw3.h>

namespace Gauntlet
{

double Timer::Now()
{
    return glfwGetTime();
}

}  // namespace Gauntlet