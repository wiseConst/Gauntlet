#pragma once

#include <GauntletPCH.h>

namespace Gauntlet
{

class Event;

struct WindowSpecification
{
  public:
    WindowSpecification()  = delete;
    ~WindowSpecification() = default;

    WindowSpecification(const std::string_view& InTitle = "Gauntlet Engine", uint32_t width = 1280, uint32_t height = 720)
        : Title(InTitle), Width(width), Height(height)
    {
    }

    std::string_view Title;
    uint32_t Width;
    uint32_t Height;
};

class Window : private Unmovable, private Uncopyable
{
  protected:
    using EventCallbackFn = std::function<void(Event&)>;

  public:
    Window(const WindowSpecification& InWindowSpec) : m_WindowSpec(InWindowSpec) {}

    Window()          = delete;
    virtual ~Window() = default;

    virtual void OnUpdate()        = 0;
    virtual void HandleMinimized() = 0;

    virtual void SetWindowLogo(const std::string_view& InFilePath) = 0;
    virtual void SetWindowTitle(const std::string_view& InTitle)   = 0;
    virtual void SetVSync(bool IsVsync)                            = 0;
    FORCEINLINE void SetIsRunning(bool IsRunning) { m_IsRunning = IsRunning; }
    FORCEINLINE virtual void SetWindowCallback(const EventCallbackFn& InFnCallback) = 0;

    FORCEINLINE bool IsRunning() const { return m_IsRunning; }
    FORCEINLINE bool IsVSync() const { return m_IsVSync; }
    FORCEINLINE bool IsMinimized() const { return m_WindowSpec.Height == 0 || m_WindowSpec.Width == 0; }

    FORCEINLINE const char* GetTitle() const { return m_WindowSpec.Title.data(); }
    virtual void* GetNativeWindow() const = 0;
    virtual uint32_t GetWidth() const { return m_WindowSpec.Width; }
    virtual uint32_t GetHeight() const { return m_WindowSpec.Height; }

    FORCEINLINE const float GetAspectRatio() { return GetWidth() / (float)GetHeight(); }

    static Window* Create(const WindowSpecification& InWindowSpec);

  protected:
    WindowSpecification m_WindowSpec;

    bool m_IsRunning{false};
    bool m_IsVSync{false};
};

}  // namespace Gauntlet