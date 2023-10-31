#pragma once
#include <Gauntlet/Core/PlatformDetection.h>

#ifdef GNT_PLATFORM_WINDOWS

namespace Gauntlet
{
extern Scoped<Application> CreateApplication(const CommandLineArguments& args);

int Main(int argc, char** argv)
{
    auto App = Gauntlet::CreateApplication({argc, argv});
    App->Run();

    return 0;
}

}  // namespace Gauntlet

#if GNT_RELEASE

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
    return Gauntlet::Main(__argc, __argv);
}

#else

int main(int argc, char** argv)
{
    return Gauntlet::Main(argc, argv);
}

#endif

#endif