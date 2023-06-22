#pragma once

#include <EclipsePCH.h>

namespace Eclipse
{

	class Event;

	struct WindowSpecification
	{
	public:
		WindowSpecification() = delete;
		~WindowSpecification() = default;

		WindowSpecification(const std::string_view& name, uint32_t width, uint32_t height) :Name(name), Width(width), Height(height)
		{}

		std::string_view Name;
		uint32_t Width;
		uint32_t Height;
	};

	class Window
	{
	protected:
		using EventCallbackFn = std::function<void(Event&)>;

	public:
		Window(const WindowSpecification& InWindowSpec) :m_WindowSpec(InWindowSpec) {}

		Window() = delete;
		virtual ~Window() = default;

		Window(const Window& other) = delete;
		Window(Window&& other) = delete;

		Window& operator=(const Window& other) = delete;
		Window& operator=(Window&& other) = delete;

		FORCEINLINE virtual void SetWindowCallback(const EventCallbackFn& InFnCallback) = 0;
		virtual void* GetNativeWindow() const = 0;

		virtual void OnUpdate() = 0;
		FORCEINLINE bool IsRunning() const { return m_IsRunning; }

		FORCEINLINE void SetIsRunning(bool IsRunning) { m_IsRunning = IsRunning; }

		static Window* Create(const WindowSpecification& InWindowSpec);
	protected:
		
		WindowSpecification m_WindowSpec;
		bool m_IsRunning{ false };
	};

}