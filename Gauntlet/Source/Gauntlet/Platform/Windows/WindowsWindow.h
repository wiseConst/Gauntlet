#pragma once

#include <Gauntlet/Core/Window.h>

struct GLFWwindow;

namespace Gauntlet
{

class WindowsWindow final : public Window
{
  public:
    WindowsWindow() = delete;
    WindowsWindow(const WindowSpecification& windowSpec);

    ~WindowsWindow();

    void OnUpdate() final override;
    void HandleMinimized() final override;

    void SetWindowLogo(const std::string_view& filePath) final override;
    void SetWindowTitle(const std::string_view& title) final override;
    void SetVSync(bool IsVsync) final override;
    FORCEINLINE void SetWindowCallback(const EventCallbackFn& fnCallback) final override { m_CallbackFn = fnCallback; }

    FORCEINLINE void* GetNativeWindow() const final override { return m_Window; }

  private:
    GLFWwindow* m_Window;
    EventCallbackFn m_CallbackFn;

    void Init();
    void Shutdown();

    void SetCallbacks();
    void SetPositionCentered();
};

}  // namespace Gauntlet