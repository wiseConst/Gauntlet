#include "EclipsePCH.h"
#include "Application.h"

#include "Log.h"
#include "Window.h"

#include "Eclipse/ImGui/ImGuiLayer.h"
#include "Eclipse/Renderer/GraphicsContext.h"
#include "Eclipse/Renderer/Renderer2D.h"

#include "Input.h"

#include "Timestep.h"
#include <GLFW/glfw3.h>

namespace Eclipse
{
Application* Application::s_Instance = nullptr;

Application::Application(const ApplicationSpecification& InApplicationSpec) : m_AppInfo(InApplicationSpec)
{
    ELS_ASSERT(!s_Instance, "You can't have 2 instances of application!");
    s_Instance = this;

    RendererAPI::Init(m_AppInfo.GraphicsAPI);

    WindowSpecification WindowSpec(InApplicationSpec.AppName, InApplicationSpec.Width, InApplicationSpec.Height);
    m_Window.reset(Window::Create(WindowSpec));
    m_Window->SetWindowCallback(BIND_EVENT_FN(Application::OnEvent));
    m_Window->SetWindowLogo(InApplicationSpec.WindowLogoPath);

    m_ThreadPool.reset(new ThreadPool());
    m_Context.reset(GraphicsContext::Create(m_Window));

    Renderer2D::Init();

    Input::Init();
    m_ImGuiLayer.reset(ImGuiLayer::Create());
    m_ImGuiLayer->OnAttach();
}

void Application::Run()
{
    LOG_INFO("Application running...");

    uint32_t FrameCount = 0;
    float LastTime      = 0.0f;
    while (m_Window->IsRunning())
    {
        const float CurrentTime = (float)glfwGetTime();
        Timestep ts             = CurrentTime - m_LastFrameTime;
        m_LastFrameTime         = CurrentTime;

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

        if (!m_Window->IsMinimized())
        {
            // Actual geometry pass
            m_Context->BeginRender();
            Renderer2D::SetClearColor({0.1f, 0.1f, 0.1f, 1.0f});

            Renderer2D::Begin();

            m_LayerQueue.OnUpdate(ts);

            Renderer2D::Flush();

            m_Context->EndRender();

            // Gui Pass
            m_ImGuiLayer->BeginRender();

            m_LayerQueue.OnImGuiRender();

            m_ImGuiLayer->EndRender();
        }

        m_Window->OnUpdate();
    }
}

void Application::OnEvent(Event& e)
{
    // LOG_TRACE("%s", e.Format().c_str());
    // TODO: EventDispatcher
    if (e.GetType() == EEventType::WindowCloseEvent || Input::IsKeyPressed(ELS_KEY_ESCAPE))
    {
        m_Window->SetIsRunning(false);
    }

    for (auto& OneLayer : m_LayerQueue)
    {
        if (OneLayer) OneLayer->OnEvent(e);
    }
}

Application::~Application()
{
    LOG_INFO("Application shutdown...");

    Input::Destroy();
    Renderer2D::Shutdown();

    m_LayerQueue.Destroy();

    m_ImGuiLayer->OnDetach();
    m_Context->Destroy();
    s_Instance = nullptr;
}

}  // namespace Eclipse