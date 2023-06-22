#pragma once

#include <Eclipse/Core/Core.h>
#include "Eclipse/Event/Event.h"
#include "Eclipse/Renderer/RendererAPI.h"

namespace Eclipse
{
	class Window;
	class GraphicsContext;

	struct ApplicationSpecification final
	{
	public:
		ApplicationSpecification() : AppName("Eclipse Engine"), Width(980), Height(540), GraphicsAPI(RendererAPI::EAPI::Vulkan) {}
		ApplicationSpecification(const std::string_view& InAppName, uint32_t InWidth, uint32_t InHeight, RendererAPI::EAPI InGraphicsAPI) :AppName(InAppName), Width(InWidth), Height(InHeight),GraphicsAPI(InGraphicsAPI) {}

		~ApplicationSpecification() = default;
		//ApplicationSpecification(const ApplicationSpecification& app) = delete;
		//ApplicationSpecification(ApplicationSpecification&& app) = delete;

		//ApplicationSpecification& operator=(const ApplicationSpecification& app) = delete;
		//ApplicationSpecification& operator=(ApplicationSpecification&& app) = delete;

		std::string_view AppName;
		uint32_t Width;
		uint32_t Height;
		RendererAPI::EAPI GraphicsAPI;
	};

	class Application
	{
	public:
		Application() = delete;
		Application(const Application& app) = delete;
		Application(Application&& app) = delete;

		Application& operator=(const Application& app) = delete;
		Application& operator=(Application&& app) = delete;

		Application(const ApplicationSpecification& InApplicationSpec = ApplicationSpecification());
		virtual ~Application();

		void Run();
		void OnEvent(Event& e);

		static Application& Get() { return *s_Instance; }

        FORCEINLINE void SubmitToGameThread(const std::function<void()>& InFunction)
        {
            std::scoped_lock<std::mutex> lock(m_GameThreadMutex);
            m_GameThreadQueue.push(InFunction);
        } 

        FORCEINLINE void SubmitToRenderThread(const std::function<void()>& InFunction)
        {
            std::scoped_lock<std::mutex> lock(m_RenderThreadMutex); 
			m_RenderThreadQueue.push(InFunction);
		}

		FORCEINLINE const auto& GetInfo() const { return m_AppInfo; }
		FORCEINLINE auto& GetInfo() { return m_AppInfo; }

	private:
		static Application* s_Instance;
		ApplicationSpecification m_AppInfo;

		Scoped<Window> m_Window;
		Scoped<GraphicsContext> m_Context;

		std::thread m_GameThread;
		std::mutex m_GameThreadMutex;
        std::queue<std::function<void()>> m_GameThreadQueue;

		std::thread m_RenderThread;
		std::mutex m_RenderThreadMutex;
        std::queue<std::function<void()>> m_RenderThreadQueue;

		void ExecuteGameThreadQueue();
		void ExecuteRenderThreadQueue();
	};

	Scoped<Application> CreateApplication();
}