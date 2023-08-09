#pragma once
#include <Gauntlet/Core/PlatformDetection.h>

#ifdef GNT_PLATFORM_WINDOWS

extern Gauntlet::Scoped<Gauntlet::Application> Gauntlet::CreateApplication();

int main(int argc, char** argv)
{
    auto App = Gauntlet::CreateApplication();
    App->Run();

    return 0;
}

#endif