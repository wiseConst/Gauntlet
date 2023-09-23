#include "GauntletPCH.h"
#include "Gauntlet/Core/Input.h"

#include "Gauntlet/Core/Application.h"
#include "Gauntlet/Core/Window.h"

#include <GLFW/glfw3.h>

namespace Gauntlet
{

static GLFWwindow* GetNativeWindow()
{
    auto* Window = static_cast<GLFWwindow*>(Application::Get().GetWindow()->GetNativeWindow());
    GNT_ASSERT(Window, "Window handle is null!");

    return Window;
}

bool Input::IsKeyPressed(KeyCode keyCode)
{
    const auto State = glfwGetKey(GetNativeWindow(), static_cast<int32_t>(keyCode));
    return State == GLFW_PRESS || State == GLFW_REPEAT;
}

bool Input::IsMouseButtonPressed(KeyCode button)
{
    const auto State = glfwGetMouseButton(GetNativeWindow(), static_cast<int32_t>(button));
    return State == GLFW_PRESS || State == GLFW_REPEAT;
}

bool Input::IsKeyReleased(KeyCode keyCode)
{
    const auto State = glfwGetKey(GetNativeWindow(), static_cast<int32_t>(keyCode));
    return State == GLFW_RELEASE;
}

bool Input::IsMouseButtonReleased(KeyCode button)
{
    const auto State = glfwGetMouseButton(GetNativeWindow(), static_cast<int32_t>(button));
    return State == GLFW_RELEASE;
}

std::pair<int32_t, int32_t> Input::GetMousePosition()
{
    std::pair<int, int> MousePosition = {0, 0};
    glfwGetWindowPos(GetNativeWindow(), &MousePosition.first, &MousePosition.second);
    return MousePosition;
}

int32_t Input::GetMouseX()
{
    const auto [x, y] = GetMousePosition();
    return x;
}

int32_t Input::GetMouseY()
{
    const auto [x, y] = GetMousePosition();
    return y;
}

}  // namespace Gauntlet