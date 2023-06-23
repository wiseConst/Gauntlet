#include <EclipsePCH.h>
#include "WindowsWindow.h"

#include "Eclipse/Event/Event.h"
#include "Eclipse/Event/MouseEvent.h"
#include "Eclipse/Event/WindowEvent.h"
#include "Eclipse/Event/KeyEvent.h"

#include "Eclipse/Renderer/RendererAPI.h"
#include "Eclipse/Core/Input.h"

#include <GLFW/glfw3.h>

namespace Eclipse
{

static bool s_GLFWInitialized = false;

static void GLFWErrorCallback(int error, const char* description)
{
    LOG_ERROR("GLFW Error (%d: %s)", error, description);
}

	Window* Window::Create(const WindowSpecification& InWindowSpec)
	{
		return new WindowsWindow(InWindowSpec);
	}

	WindowsWindow::WindowsWindow(const WindowSpecification& InWindowSpec) : Window(InWindowSpec)
	{
		Init();
		m_IsRunning = true;
	}

	void WindowsWindow::Init()
	{
        LOG_INFO("Creating window %s (%u, %u)", m_WindowSpec.Name.data(), m_WindowSpec.Width, m_WindowSpec.Height);

        if (!s_GLFWInitialized)
        {
            int success = glfwInit();
            ELS_ASSERT(success, "Could not init GLFW!");
            s_GLFWInitialized = true;

            glfwSetErrorCallback(GLFWErrorCallback);
        }

        switch (RendererAPI::Get())
        {
            case RendererAPI::EAPI::Vulkan:
            {
                glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
                break;
            }
        }

		m_Window = glfwCreateWindow(static_cast<int>(m_WindowSpec.Width), static_cast<int>(m_WindowSpec.Height), m_WindowSpec.Name.data(), nullptr, nullptr);

		glfwSetWindowUserPointer(m_Window, this);
		SetCallbacks();
	}

	void WindowsWindow::SetCallbacks()
	{
		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double x, double y)
			{
				auto& Window = *(WindowsWindow*)glfwGetWindowUserPointer(window);

				MouseMovedEvent e((int)x, (int)y);
				Window.m_CallbackFn(e);
			});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xoffset, double yoffset) {
			auto& Window = *(WindowsWindow*)glfwGetWindowUserPointer(window);

			MouseScrolledEvent e((int)xoffset, (int)yoffset);
			Window.m_CallbackFn(e);
			});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods) {
			auto& Window = *(WindowsWindow*)glfwGetWindowUserPointer(window);

			switch (action)
			{
			case GLFW_PRESS:
			{
				MouseButtonPressedEvent e(button, action);
				Window.m_CallbackFn(e);
				break;
			}
			case GLFW_RELEASE:
			{
				MouseButtonReleasedEvent e(button, action);
				Window.m_CallbackFn(e);
				break;
			}
			case GLFW_REPEAT:
			{
				MouseButtonRepeatedEvent e(button, action, 2);
				Window.m_CallbackFn(e);
				break;
			}
			}});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			auto& Window = *(WindowsWindow*)glfwGetWindowUserPointer(window);

			switch (action)
			{
			case GLFW_PRESS:
			{
				KeyButtonPressedEvent e(key, scancode);
				Window.m_CallbackFn(e);
				break;
			}
			case GLFW_RELEASE:
			{
				KeyButtonReleasedEvent e(key, scancode);
				Window.m_CallbackFn(e);
				break;
			}
			case GLFW_REPEAT:
			{
				KeyButtonRepeatedEvent e(key, scancode, action, 2);
				Window.m_CallbackFn(e);
				break;
			}
			}
			});

		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
			auto& Window = *(WindowsWindow*)glfwGetWindowUserPointer(window);

			WindowResizeEvent e(width, height);
			Window.m_WindowSpec.Width = (uint32_t)width;
			Window.m_WindowSpec.Height = (uint32_t)height;
			Window.m_CallbackFn(e);
			});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
			auto& Window = *(WindowsWindow*)glfwGetWindowUserPointer(window);

			WindowCloseEvent e;
			Window.m_CallbackFn(e);
			});
	}

	void WindowsWindow::OnUpdate()
	{
		glfwPollEvents();

		if (Input::IsKeyPressed(ELS_KEY_ESCAPE))
		{
			m_IsRunning = false;
		}
	}

	void WindowsWindow::Shutdown()
	{
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

	WindowsWindow::~WindowsWindow()
	{
		Shutdown();
	}

}