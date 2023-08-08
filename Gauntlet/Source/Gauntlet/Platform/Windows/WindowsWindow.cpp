#include <GauntletPCH.h>
#include "WindowsWindow.h"

#include "Gauntlet/Event/Event.h"
#include "Gauntlet/Event/KeyEvent.h"
#include "Gauntlet/Event/MouseEvent.h"
#include "Gauntlet/Event/WindowEvent.h"

#include "Gauntlet/Renderer/RendererAPI.h"
#include "Gauntlet/Renderer/GraphicsContext.h"
#include "Gauntlet/Renderer/Image.h"
#include "Gauntlet/Core/Input.h"

#include <GLFW/glfw3.h>

namespace Gauntlet
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
    LOG_INFO("Creating window %s (%u, %u)", m_WindowSpec.Title.data(), m_WindowSpec.Width, m_WindowSpec.Height);

    if (!s_GLFWInitialized)
    {
        int success = glfwInit();
        GNT_ASSERT(success, "Could not init GLFW!");
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

    m_Window = glfwCreateWindow(static_cast<int>(m_WindowSpec.Width), static_cast<int>(m_WindowSpec.Height), m_WindowSpec.Title.data(),
                                nullptr, nullptr);

    glfwSetWindowUserPointer(m_Window, this);
    SetCallbacks();
}

void WindowsWindow::SetCallbacks()
{
    glfwSetCursorPosCallback(m_Window,
                             [](GLFWwindow* window, double x, double y)
                             {
                                 auto& Window = *(WindowsWindow*)glfwGetWindowUserPointer(window);

                                 MouseMovedEvent e((int)x, (int)y);
                                 Window.m_CallbackFn(e);
                             });

    glfwSetScrollCallback(m_Window,
                          [](GLFWwindow* window, double xoffset, double yoffset)
                          {
                              auto& Window = *(WindowsWindow*)glfwGetWindowUserPointer(window);

                              MouseScrolledEvent e((int)xoffset, (int)yoffset);
                              Window.m_CallbackFn(e);
                          });

    glfwSetMouseButtonCallback(m_Window,
                               [](GLFWwindow* window, int button, int action, int mods)
                               {
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
                                   }
                               });

    glfwSetKeyCallback(m_Window,
                       [](GLFWwindow* window, int key, int scancode, int action, int mods)
                       {
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

    glfwSetWindowSizeCallback(m_Window,
                              [](GLFWwindow* window, int width, int height)
                              {
                                  auto& Window = *(WindowsWindow*)glfwGetWindowUserPointer(window);

                                  WindowResizeEvent e(width, height);
                                  Window.m_WindowSpec.Width  = (uint32_t)width;
                                  Window.m_WindowSpec.Height = (uint32_t)height;
                                  Window.m_CallbackFn(e);
                              });

    glfwSetWindowCloseCallback(m_Window,
                               [](GLFWwindow* window)
                               {
                                   auto& Window = *(WindowsWindow*)glfwGetWindowUserPointer(window);

                                   WindowCloseEvent e;
                                   Window.m_CallbackFn(e);
                               });
}

void WindowsWindow::OnUpdate()
{
    glfwPollEvents();

    if (!IsMinimized())
    {
        auto& Context = GraphicsContext::Get();
        Context.SwapBuffers();
    }
}

void WindowsWindow::HandleMinimized()
{
    int32_t Width{0}, Height{0};
    glfwGetFramebufferSize(m_Window, &Width, &Height);
    while (Width == 0 || Height == 0 || !IsRunning())
    {
        glfwWaitEvents();
        glfwGetFramebufferSize(m_Window, &Width, &Height);
    }
}

void WindowsWindow::SetWindowLogo(const std::string_view& InFilePath)
{
    int32_t Width    = 0;
    int32_t Height   = 0;
    int32_t Channels = 0;
    stbi_uc* Pixels  = ImageUtils::LoadImageFromFile(InFilePath.data(), &Width, &Height, &Channels);

    GLFWimage IconImage = {};
    IconImage.pixels    = Pixels;
    IconImage.width     = Width;
    IconImage.height    = Height;

    glfwSetWindowIcon(m_Window, 1, &IconImage);
    ImageUtils::UnloadImage(Pixels);
}

void WindowsWindow::SetVSync(bool IsVsync)
{
    if (m_IsVSync == IsVsync) return;
    m_IsVSync = IsVsync;

    auto& Context = GraphicsContext::Get();
    Context.SetVSync(IsVsync);
}

void WindowsWindow::SetWindowTitle(const std::string_view& InTitle)
{
    GNT_ASSERT(InTitle.data(), "Window title is not valid!");

    glfwSetWindowTitle(m_Window, InTitle.data());
    m_WindowSpec.Title = InTitle;
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

}  // namespace Gauntlet