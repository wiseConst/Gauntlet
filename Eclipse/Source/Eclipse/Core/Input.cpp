#include "EclipsePCH.h"
#include "Input.h"

#include "Platform/Windows/WindowsInput.h"
//#include "Platform/Linux/LinuxInput.h"

namespace Eclipse
{
Input* Input::s_Instance = nullptr;

void Input::Init()
{
    ELS_ASSERT(!s_Instance, "Input already initalized!");

#ifdef ELS_PLATFORM_WINDOWS
    s_Instance = new WindowsInput();
#elif defined(ELS_PLATFORM_LINUX)
    s_Instance = new LinuxInput();
#endif
}
}  // namespace Eclipse