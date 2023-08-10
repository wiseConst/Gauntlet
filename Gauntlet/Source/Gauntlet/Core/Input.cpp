#include "GauntletPCH.h"
#include "Input.h"

namespace Gauntlet
{
Input* Input::s_Instance = nullptr;

void Input::Init()
{
    GNT_ASSERT(!s_Instance, "Input already initalized!");
    s_Instance = new Input();
}

void Input::Destroy()
{
    delete Input::s_Instance;
}

}  // namespace Gauntlet