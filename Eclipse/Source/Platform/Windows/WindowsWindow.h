#pragma once

#include <Eclipse/Core/Window.h>

struct GLFWwindow;

namespace Eclipse
{

class WindowsWindow final : public Window
{
  public:
    WindowsWindow() = delete;
    WindowsWindow(const WindowSpecification& InWindowSpec);

    ~WindowsWindow();

    FORCEINLINE void SetWindowCallback(const EventCallbackFn& InFnCallback) final override { m_CallbackFn = InFnCallback; }
    void OnUpdate() final override;

    void HandleMinimized() final override;
    void SetWindowLogo(const std::string_view& InFilePath) final override;
    void SetVSync(bool IsVsync) final override;

    FORCEINLINE void* GetNativeWindow() const final override { return m_Window; }

  private:
    GLFWwindow* m_Window;
    EventCallbackFn m_CallbackFn;

    void Init();
    void Shutdown();

    void SetCallbacks();
};

}  // namespace Eclipse