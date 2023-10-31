#pragma once

#include <Gauntlet/Core/Core.h>

#include "Gauntlet/Event/Event.h"
#include "Gauntlet/Renderer/RendererAPI.h"

#include "Gauntlet/Layer/LayerQueue.h"

namespace Gauntlet
{
class Window;
class GraphicsContext;
class ImGuiLayer;
class WindowCloseEvent;

struct CommandLineArguments
{
    int32_t argc = 0;
    char** argv  = nullptr;
};

struct ApplicationSpecification final
{
  public:
    ApplicationSpecification()
        : AppName("Gauntlet Engine"), WindowLogoPath(), Width(1280), Height(720), GraphicsAPI(RendererAPI::EAPI::Vulkan)
    {
    }

    ~ApplicationSpecification() = default;

    std::string_view AppName;
    std::string_view WindowLogoPath;
    CommandLineArguments CmdArgs = {};
    uint32_t Width;
    uint32_t Height;
    RendererAPI::EAPI GraphicsAPI;
    bool bUseCustomTitleBar = false;
    uint32_t FPSLock        = 0;  // "0" means no lock at all
};

class Application : private Uncopyable, private Unmovable
{
  public:
    Application() = delete;
    Application(const ApplicationSpecification& ApplicationSpec = ApplicationSpecification());
    virtual ~Application();

    void Run();
    void OnEvent(Event& e);

    FORCEINLINE static Application& Get() { return *s_Instance; }
    FORCEINLINE auto& GetGUILayer() { return m_ImGuiLayer; }

    FORCEINLINE const auto& GetSpecification() const { return m_Specification; }
    FORCEINLINE auto& GetSpecification() { return m_Specification; }

    FORCEINLINE const auto& GetWindow() const { return m_Window; }
    FORCEINLINE auto& GetWindow() { return m_Window; }

    FORCEINLINE void PushLayer(Layer* InLayer) { m_LayerQueue.Enqueue(InLayer); }
    static const float GetTimeNow();

    void Close();

  private:
    static Application* s_Instance;
    ApplicationSpecification m_Specification;

    Scoped<Window> m_Window;
    Scoped<GraphicsContext> m_Context;
    Scoped<ImGuiLayer> m_ImGuiLayer;

    LayerQueue m_LayerQueue;
    float m_MainThreadDelta = 0.0f;
    const std::thread::id m_MainThreadID;

    void OnWindowClosed(WindowCloseEvent& InEvent);
};

Scoped<Application> CreateApplication(const CommandLineArguments& args);
}  // namespace Gauntlet