#pragma once
#include <Gauntlet/Core/PlatformDetection.h>

#ifdef GNT_PLATFORM_WINDOWS

namespace Gauntlet
{
extern Scoped<Application> CreateApplication(const CommandLineArguments& args);

int32_t Main(int32_t argc, char** argv)
{
    auto App = Gauntlet::CreateApplication({argc, argv});
    App->Run();

    std::quick_exit(0);
    return 0;
}

}  // namespace Gauntlet

#if GNT_RELEASE

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
    return Gauntlet::Main(__argc, __argv);
}

#else

int32_t main(int32_t argc, char** argv)
{
    return Gauntlet::Main(argc, argv);
}

#endif

#endif