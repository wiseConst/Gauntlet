#pragma once

#include "Eclipse/Core/Input.h"

struct GLFWwindow;

namespace Eclipse
{

class WindowsInput final : public Input
{
  public:
    WindowsInput()  = default;
    ~WindowsInput() = default;

  protected:
    bool IsKeyPressedImpl(int KeyCode) const final override;
    bool IsMouseButtonPressedImpl(int Button) const final override;

    bool IsKeyReleasedImpl(int KeyCode) const final override;
    bool IsMouseButtonReleasedImpl(int Button) const final override;

    std::pair<int, int> GetMousePositionImpl() const final override;
    int GetMouseXImpl() const final override;
    int GetMouseYImpl() const final override;

    void DestroyImpl() final override;

  private:
    GLFWwindow* GetNativeWindow() const;
};

}  // namespace Eclipse