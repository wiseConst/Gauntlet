#pragma once

#include <Eclipse/Core/Window.h>

struct GLFWwindow;

namespace Eclipse
{

	class WindowsWindow final : public Window
	{
	public:
		WindowsWindow() = delete;
		WindowsWindow(const WindowSpecification& InWindowSpec);

		virtual ~WindowsWindow();

		FORCEINLINE virtual void SetWindowCallback(const EventCallbackFn& InFnCallback) override final{ m_CallbackFn = InFnCallback; }
		virtual void OnUpdate() override final;

		FORCEINLINE virtual void* GetNativeWindow() const override final { return m_Window; }
	private:
		GLFWwindow* m_Window;
		EventCallbackFn m_CallbackFn;

		void Init();
		void Shutdown();
	
		void SetCallbacks();
	};


}