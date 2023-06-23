#include <EclipsePCH.h>
#include "Application.h"

#include "Log.h"
#include "Window.h"

#include "Eclipse/Renderer/Renderer.h"
#include "Eclipse/Renderer/GraphicsContext.h"
#include "Input.h"

namespace Eclipse
{
	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationSpecification& InApplicationSpec) :m_AppInfo(InApplicationSpec)
	{
		ELS_ASSERT(s_Instance == nullptr, "You can't have 2 instances of application!");
		s_Instance = this;

		Renderer::Init(InApplicationSpec.GraphicsAPI);

		// Window creation
		WindowSpecification WindowSpec(InApplicationSpec.AppName, InApplicationSpec.Width, InApplicationSpec.Height);
		m_Window.reset(Window::Create(WindowSpec));
		m_Window->SetWindowCallback(BIND_FN(Application::OnEvent));

		m_Context.reset(GraphicsContext::Create(m_Window));

		Input::Init();
	}

	Application::~Application()
	{
        Input::Destroy();
        Renderer::Shutdown();

		m_Context->Destroy();
	}

	void Application::Run()
	{
		LOG_INFO("Application running! Using 2 threads: GameThread and RenderThread.");

		while (m_Window->IsRunning())
		{
            //ExecuteGameThreadQueue();

            m_Context->BeginRender();

			//ExecuteRenderThreadQueue();

			m_Context->EndRender();

			m_Window->OnUpdate();
		}
	}

	void Application::OnEvent(Event& e)
	{
		if (e.GetType() == Event::EventType::WindowCloseEvent)
		{
			m_Window->SetIsRunning(false);
		}

		LOG_TRACE("%s", e.Format().c_str());
    }

    void Application::ExecuteGameThreadQueue() {

		while (m_GameThreadQueue.size() > 0)
		{
            std::scoped_lock<std::mutex> lock(m_GameThreadMutex);
            auto& func = m_GameThreadQueue.front();
            m_GameThread = std::thread(func);

			m_GameThread.join();

			m_GameThreadQueue.pop();
		}
	}

    void Application::ExecuteRenderThreadQueue() {

        while (m_RenderThreadQueue.size() > 0)
        {
            std::scoped_lock<std::mutex> lock(m_RenderThreadMutex);
            auto& func = m_RenderThreadQueue.front();
            m_RenderThread = std::thread(func);

			m_RenderThread.join();

            m_RenderThreadQueue.pop();
        }
	}

}