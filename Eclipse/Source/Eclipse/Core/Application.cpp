#include "EclipsePCH.h"
#include "Application.h"

#include "Log.h"
#include "Window.h"

#include "Eclipse/Renderer/GraphicsContext.h"
#include "Eclipse/Renderer/Renderer.h"
#include "Input.h"

namespace Eclipse
{
Application* Application::s_Instance = nullptr;

Application::Application(const ApplicationSpecification& InApplicationSpec) : m_AppInfo(InApplicationSpec)
{
    ELS_ASSERT(!s_Instance, "You can't have 2 instances of application!");
    s_Instance = this;

    Renderer::Init(InApplicationSpec.GraphicsAPI);

    WindowSpecification WindowSpec(InApplicationSpec.AppName, InApplicationSpec.Width, InApplicationSpec.Height);
    m_Window.reset(Window::Create(WindowSpec));
    m_Window->SetWindowCallback(BIND_EVENT_FN(Application::OnEvent));
    m_Window->SetWindowLogo(InApplicationSpec.WindowLogoPath);

    m_Context.reset(GraphicsContext::Create(m_Window));
    m_ThreadPool.reset(new ThreadPool());

    Input::Init();
}

void Application::Run()
{
    LOG_INFO("Application running!");

    while (m_Window->IsRunning())
    {
        m_Timestep.Update();
        // LOG_INFO("dt: %fms", m_Timestep.GetDeltaTime());

        if (!m_Window->IsMinimized())
        {
            m_Context->BeginRender();

            m_LayerQueue.OnUpdate(m_Timestep);

            m_Context->EndRender();
        }

        m_Window->OnUpdate();
    }
}

void Application::OnEvent(Event& e)
{
    if (e.GetType() == Event::EventType::WindowCloseEvent)
    {
        m_Window->SetIsRunning(false);
    }

    // LOG_TRACE("%s", e.Format().c_str());
}

Application::~Application()
{
    Input::Destroy();
    Renderer::Shutdown();

    m_Context->Destroy();
}

}  // namespace Eclipse