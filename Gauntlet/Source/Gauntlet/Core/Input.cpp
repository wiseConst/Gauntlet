#include "GauntletPCH.h"
#include "Input.h"

#include "Gauntlet/Platform/Windows/WindowsInput.h"
//#include "Platform/Linux/LinuxInput.h"

namespace Gauntlet
{
Input* Input::s_Instance = nullptr;

void Input::Init()
{
    GNT_ASSERT(!s_Instance, "Input already initalized!");

#ifdef ELS_PLATFORM_WINDOWS
    s_Instance = new WindowsInput();
#elif defined(ELS_PLATFORM_LINUX)
    s_Instance = new LinuxInput();
#endif
}
}  // namespace Gauntlet