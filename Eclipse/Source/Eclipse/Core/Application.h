#pragma once

#include <Eclipse/Core/Core.h>

#include "Eclipse/Event/Event.h"
#include "Eclipse/Renderer/RendererAPI.h"

#include "Eclipse/Layer/LayerQueue.h"
#include "ThreadPool.h"

namespace Eclipse
{
class Window;
class GraphicsContext;
class ImGuiLayer;

struct ApplicationSpecification final
{
  public:
    ApplicationSpecification()
        : AppName("Eclipse Engine"), WindowLogoPath("Resources/Logo/Eclipse.jpg"), Width(1280), Height(720),
          GraphicsAPI(RendererAPI::EAPI::Vulkan)
    {
    }
    ApplicationSpecification(const std::string_view& InAppName, const std::string_view& InWindowLogoPath, uint32_t InWidth,
                             uint32_t InHeight, RendererAPI::EAPI InGraphicsAPI)
        : AppName(InAppName), WindowLogoPath(InWindowLogoPath), Width(InWidth), Height(InHeight), GraphicsAPI(InGraphicsAPI)
    {
    }

    ~ApplicationSpecification() = default;

    std::string_view AppName;
    std::string_view WindowLogoPath;
    uint32_t Width;
    uint32_t Height;
    RendererAPI::EAPI GraphicsAPI;
};

class Application : private Uncopyable, private Unmovable
{
  public:
    Application() = delete;
    Application(const ApplicationSpecification& InApplicationSpec = ApplicationSpecification());
    virtual ~Application();

    void Run();
    void OnEvent(Event& e);

    FORCEINLINE static Application& Get() { return *s_Instance; }

    FORCEINLINE const auto& GetThreadPool() const { return m_ThreadPool; }
    FORCEINLINE auto& GetThreadPool() { return m_ThreadPool; }

    FORCEINLINE const auto& GetInfo() const { return m_AppInfo; }
    FORCEINLINE auto& GetInfo() { return m_AppInfo; }

    FORCEINLINE const auto& GetWindow() const { return m_Window; }
    FORCEINLINE auto& GetWindow() { return m_Window; }

    FORCEINLINE void PushLayer(Layer* InLayer) { m_LayerQueue.Enqueue(InLayer); }

  private:
    static Application* s_Instance;
    ApplicationSpecification m_AppInfo;

    Scoped<Window> m_Window;
    Scoped<GraphicsContext> m_Context;
    Scoped<ThreadPool> m_ThreadPool;
    Scoped<ImGuiLayer> m_ImGuiLayer;

    LayerQueue m_LayerQueue;
    float m_LastFrameTime = 0.0f;
};

Scoped<Application> CreateApplication();
}  // namespace Eclipse