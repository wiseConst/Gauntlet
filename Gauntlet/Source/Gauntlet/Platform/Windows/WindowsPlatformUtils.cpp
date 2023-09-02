#include "GauntletPCH.h"
#include "Gauntlet/Utils/PlatformUtils.h"

#include <commdlg.h>
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "Gauntlet/Core/Application.h"
#include "Gauntlet/Core/Window.h"

namespace Gauntlet
{

std::string FileDialogs::OpenFile(const std::string_view& filter)
{
    OPENFILENAMEA ofn = {};
    CHAR szFile[260]  = {0};

    ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
    ofn.lStructSize  = sizeof(OPENFILENAMEA);
    ofn.hwndOwner    = glfwGetWin32Window(static_cast<GLFWwindow*>(Application::Get().GetWindow()->GetNativeWindow()));
    ofn.lpstrFile    = szFile;
    ofn.nMaxFile     = sizeof(szFile);
    ofn.lpstrFilter  = filter.data();
    ofn.nFilterIndex = 1;
    ofn.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn) == TRUE)
    {
        return ofn.lpstrFile;
    }

    return std::string();
}

std::string FileDialogs::SaveFile(const std::string_view& filter)
{
    OPENFILENAMEA ofn = {};
    CHAR szFile[260]  = {0};

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize  = sizeof(OPENFILENAME);
    ofn.hwndOwner    = glfwGetWin32Window(static_cast<GLFWwindow*>(Application::Get().GetWindow()->GetNativeWindow()));
    ofn.lpstrFile    = szFile;
    ofn.nMaxFile     = sizeof(szFile);
    ofn.lpstrFilter  = filter.data();
    ofn.nFilterIndex = 1;
    ofn.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetSaveFileNameA(&ofn) == TRUE)
    {
        return ofn.lpstrFile;
    }

    return std::string();
}

}  // namespace Gauntlet