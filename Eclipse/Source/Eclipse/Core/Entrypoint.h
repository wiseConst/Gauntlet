#pragma once
#include <Eclipse/Core/PlatformDetection.h>

#ifdef ELS_PLATFORM_WINDOWS

extern Eclipse::Scoped<Eclipse::Application> Eclipse::CreateApplication();

int main(int argc, char** argv)
{
    auto App = Eclipse::CreateApplication();
    App->Run();

    return 0;
}

#endif