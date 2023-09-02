#include "GauntletPCH.h"
#include "Application.h"

#include "Log.h"
#include "Window.h"
#include "Input.h"
#include "JobSystem.h"

#include "Gauntlet/ImGui/ImGuiLayer.h"
#include "Gauntlet/Renderer/GraphicsContext.h"
#include "Gauntlet/Renderer/Renderer2D.h"
#include "Gauntlet/Renderer/Renderer.h"

#include "Timestep.h"
#include <GLFW/glfw3.h>

namespace Gauntlet
{
Application* Application::s_Instance = nullptr;

Application::Application(const ApplicationSpecification& ApplicationSpec)
    : m_Specification(ApplicationSpec), m_MainThreadID(std::this_thread::get_id())
{
    GNT_ASSERT(!s_Instance, "You can't have 2 instances of application!");
    s_Instance = this;

    Log::Init();

    JobSystem::Init();
    RendererAPI::Init(m_Specification.GraphicsAPI);

    WindowSpecification WindowSpec(m_Specification.AppName, m_Specification.Width, m_Specification.Height);
    m_Window.reset(Window::Create(WindowSpec));
    m_Window->SetWindowCallback(BIND_FN(Application::OnEvent));
    m_Window->SetWindowLogo(m_Specification.WindowLogoPath);

    std::string ConfigurationString;
#if GNT_DEBUG
    ConfigurationString = "Debug";
#else
    ConfigurationString = "Release";
#endif

    const auto RHIString = RendererAPI::Get() == RendererAPI::EAPI::Vulkan ? "Vulkan" : "None";
    const auto WindowTitle =
        std::string(m_Window->GetTitle()) + std::string(" - ") + ConfigurationString + std::string(" <") + RHIString + std::string(">");
    m_Window->SetWindowTitle(WindowTitle);

    m_Context.reset(GraphicsContext::Create(m_Window));

    Renderer::Init();
    Renderer2D::Init();

    m_ImGuiLayer = ImGuiLayer::Create();
    m_ImGuiLayer->OnAttach();
}

void Application::Run()
{
    m_LayerQueue.Init();

    {
        const float AppPrepareStartTime = GetTimeNow();
        JobSystem::Wait();  // JobSystem::Update();
        const float AppPrepareEndTime = GetTimeNow();
        LOG_INFO("Time took to prepare application: (%f)ms", AppPrepareEndTime - AppPrepareStartTime);
    }

    uint32_t FrameCount = 0;
    float LastTime      = 0.0f;
    float LastFrameTime = 0.0f;
    while (m_Window->IsRunning())
    {
        JobSystem::Update();

        if (!m_Window->IsMinimized())
        {
            m_Context->BeginRender();

            Renderer::Begin();
            Renderer2D::Begin();

            m_LayerQueue.OnUpdate(m_MainThreadDelta);

            Renderer2D::Flush();
            Renderer::Flush();

            m_Context->EndRender();

            m_ImGuiLayer->BeginRender();

            m_LayerQueue.OnImGuiRender();

            m_ImGuiLayer->EndRender();
        }

        m_Window->OnUpdate();

        // MainThread delta
        const float CurrentTime = GetTimeNow();
        m_MainThreadDelta       = CurrentTime - LastFrameTime;
        LastFrameTime           = CurrentTime;

        // FPS
        const float DeltaTime = CurrentTime - LastTime;
        ++FrameCount;
        if (DeltaTime >= 1.0f)
        {
            Renderer::GetStats().FPS = static_cast<uint32_t>(FrameCount / DeltaTime);
            FrameCount               = 0;
            LastTime                 = CurrentTime;
        }
    }
}

void Application::OnEvent(Event& e)
{
    if (m_Window->IsMinimized()) return;
    if (Input::IsKeyPressed(KeyCode::KEY_ESCAPE)) OnWindowClosed((WindowCloseEvent&)e);

    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowCloseEvent>(BIND_FN(Application::OnWindowClosed));

    m_ImGuiLayer->OnEvent(e);
    if (e.IsHandled()) return;

    for (auto& OneLayer : m_LayerQueue)
    {
        if (OneLayer) OneLayer->OnEvent(e);
    }
}

const float Application::GetTimeNow()
{
    return static_cast<float>(glfwGetTime());
}

void Application::Close()
{
    m_Window->SetIsRunning(false);
}

void Application::OnWindowClosed(WindowCloseEvent& InEvent)
{
    m_Window->SetIsRunning(false);
}

Application::~Application()
{
    JobSystem::Shutdown();

    Renderer2D::Shutdown();
    Renderer::Shutdown();

    m_LayerQueue.Destroy();
    m_ImGuiLayer->OnDetach();

    m_Context->Destroy();

    Log::Shutdown();
    s_Instance = nullptr;
}

}  // namespace Gauntlet