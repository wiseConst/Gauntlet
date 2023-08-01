#include "EclipsePCH.h"
#include "WindowsInput.h"

#include "Eclipse/Core/Application.h"
#include "Eclipse/Core/Window.h"

#include <GLFW/glfw3.h>

namespace Eclipse
{

bool WindowsInput::IsKeyPressedImpl(int KeyCode) const
{
    const auto State = glfwGetKey(GetNativeWindow(), KeyCode);
    return State == GLFW_PRESS || State == GLFW_REPEAT;
}

bool WindowsInput::IsMouseButtonPressedImpl(int Button) const
{
    const auto State = glfwGetMouseButton(GetNativeWindow(), Button);
    return State == GLFW_PRESS || State == GLFW_REPEAT;
}

bool WindowsInput::IsKeyReleasedImpl(int KeyCode) const
{
    const auto State = glfwGetKey(GetNativeWindow(), KeyCode);
    return State == GLFW_RELEASE;
}

bool WindowsInput::IsMouseButtonReleasedImpl(int Button) const
{
    const auto State = glfwGetMouseButton(GetNativeWindow(), Button);
    return State == GLFW_RELEASE;
}

std::pair<int, int> WindowsInput::GetMousePositionImpl() const
{
    std::pair<int, int> MousePosition = {0, 0};
    glfwGetWindowPos(GetNativeWindow(), &MousePosition.first, &MousePosition.second);
    return MousePosition;
}

int WindowsInput::GetMouseXImpl() const
{
    const auto [x, y] = GetMousePositionImpl();
    return x;
}

int WindowsInput::GetMouseYImpl() const
{
    const auto [x, y] = GetMousePositionImpl();
    return y;
}

void WindowsInput::DestroyImpl()
{
    delete Input::s_Instance;
}

GLFWwindow* WindowsInput::GetNativeWindow() const
{
    auto* Window = static_cast<GLFWwindow*>(Application::Get().GetWindow()->GetNativeWindow());
    ELS_ASSERT(Window, "Window handle is null!");

    return Window;
}

}  // namespace Eclipse