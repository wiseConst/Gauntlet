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

Application::Application(const ApplicationSpecification& InApplicationSpec)
    : m_AppInfo(InApplicationSpec), m_MainThreadID(std::this_thread::get_id())
{
    GNT_ASSERT(!s_Instance, "You can't have 2 instances of application!");
    s_Instance = this;

    RendererAPI::Init(m_AppInfo.GraphicsAPI);

    WindowSpecification WindowSpec(InApplicationSpec.AppName, InApplicationSpec.Width, InApplicationSpec.Height);
    m_Window.reset(Window::Create(WindowSpec));
    m_Window->SetWindowCallback(BIND_EVENT_FN(Application::OnEvent));
    m_Window->SetWindowLogo(InApplicationSpec.WindowLogoPath);

    Input::Init();
    m_Context.reset(GraphicsContext::Create(m_Window));

    Renderer::Init();
    Renderer2D::Init();

    m_ImGuiLayer.reset(ImGuiLayer::Create());
    m_ImGuiLayer->OnAttach();

    JobSystem::Init();
}

void Application::Run()
{
    uint32_t FrameCount = 0;
    float LastTime      = 0.0f;
    float LastFrameTime = 0.0f;
    while (m_Window->IsRunning())
    {
        if (!m_Window->IsMinimized())
        {
            m_Context->BeginRender();

            Renderer::Begin();
            // Renderer2D::Begin();

            m_LayerQueue.OnUpdate(m_MainThreadDelta);

            // Renderer2D::Flush();
            Renderer::Flush();

            m_Context->EndRender();

            m_ImGuiLayer->BeginRender();

            m_LayerQueue.OnImGuiRender();

            m_ImGuiLayer->EndRender();
        }

        m_Window->OnUpdate();

        // MainThread delta
        const float CurrentTime = (float)glfwGetTime();
        m_MainThreadDelta       = CurrentTime - LastFrameTime;
        LastFrameTime           = CurrentTime;

        // FrameTime
        const float DeltaTime = CurrentTime - LastTime;
        ++FrameCount;
        if (DeltaTime >= 1.0f)
        {
            const uint32_t FPS     = FrameCount / DeltaTime;
            const auto WindowTitle = std::string(m_AppInfo.AppName) + " " + std::to_string(FPS) + " FPS";
            m_Window->SetWindowTitle(WindowTitle.data());
            FrameCount = 0;
            LastTime   = CurrentTime;
        }
    }
}

void Application::OnEvent(Event& e)
{
    if (m_Window->IsMinimized()) return;
    if (Input::IsKeyPressed(ELS_KEY_ESCAPE)) OnWindowClosed((WindowCloseEvent&)e);

    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClosed));

    for (auto& OneLayer : m_LayerQueue)
    {
        if (OneLayer) OneLayer->OnEvent(e);
    }

    m_ImGuiLayer->OnEvent(e);
}

const float Application::GetTimeNow() const
{
    return static_cast<float>(glfwGetTime());
}

void Application::OnWindowClosed(WindowCloseEvent& InEvent)
{
    m_Window->SetIsRunning(false);
}

Application::~Application()
{
    JobSystem::Shutdown();

    Input::Destroy();
    Renderer2D::Shutdown();
    Renderer::Shutdown();

    m_LayerQueue.Destroy();
    m_ImGuiLayer->OnDetach();

    m_Context->Destroy();
    s_Instance = nullptr;
}

}  // namespace Gauntlet