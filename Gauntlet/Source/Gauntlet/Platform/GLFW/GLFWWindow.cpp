#include <GauntletPCH.h>
#include "GLFWWindow.h"

#include "Gauntlet/Event/Event.h"
#include "Gauntlet/Event/KeyEvent.h"
#include "Gauntlet/Event/MouseEvent.h"
#include "Gauntlet/Event/WindowEvent.h"

#include "Gauntlet/Renderer/RendererAPI.h"
#include "Gauntlet/Renderer/Renderer.h"
#include "Gauntlet/Renderer/GraphicsContext.h"
#include "Gauntlet/Renderer/Image.h"
#include "Gauntlet/Core/Input.h"

#include <GLFW/glfw3.h>

#pragma warning(disable : 4834)

namespace Gauntlet
{

static bool s_GLFWInitialized = false;

static void GLFWErrorCallback(int error, const char* description)
{
    LOG_ERROR("GLFW Error (%d: %s)", error, description);
}

Window* Window::Create(const WindowSpecification& windowSpec)
{
    return new GLFWWindow(windowSpec);
}

GLFWWindow::GLFWWindow(const WindowSpecification& windowSpec) : Window(windowSpec)
{
    Init();
    m_bIsRunning = true;
}

void GLFWWindow::Init()
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

    SetPositionCentered();
}

void GLFWWindow::SetCallbacks()
{
    glfwSetCursorPosCallback(m_Window,
                             [](GLFWwindow* window, double x, double y)
                             {
                                 auto& Window = *(GLFWWindow*)glfwGetWindowUserPointer(window);

                                 MouseMovedEvent e((int)x, (int)y);
                                 Window.m_CallbackFn(e);
                             });

    glfwSetScrollCallback(m_Window,
                          [](GLFWwindow* window, double xoffset, double yoffset)
                          {
                              auto& Window = *(GLFWWindow*)glfwGetWindowUserPointer(window);

                              MouseScrolledEvent e((int)xoffset, (int)yoffset);
                              Window.m_CallbackFn(e);
                          });

    glfwSetMouseButtonCallback(m_Window,
                               [](GLFWwindow* window, int button, int action, int mods)
                               {
                                   auto& Window = *(GLFWWindow*)glfwGetWindowUserPointer(window);
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
                           auto& Window = *(GLFWWindow*)glfwGetWindowUserPointer(window);

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
                                   KeyButtonPressedEvent e(key, scancode, true);
                                   Window.m_CallbackFn(e);
                                   break;
                               }
                           }
                       });

    glfwSetWindowSizeCallback(m_Window,
                              [](GLFWwindow* window, int width, int height)
                              {
                                  auto& Window = *(GLFWWindow*)glfwGetWindowUserPointer(window);

                                  WindowResizeEvent e(width, height);
                                  Window.m_WindowSpec.Width  = (uint32_t)width;
                                  Window.m_WindowSpec.Height = (uint32_t)height;
                                  Window.m_CallbackFn(e);
                              });

    glfwSetWindowCloseCallback(m_Window,
                               [](GLFWwindow* window)
                               {
                                   auto& Window = *(GLFWWindow*)glfwGetWindowUserPointer(window);

                                   WindowCloseEvent e;
                                   Window.m_CallbackFn(e);
                               });
}

void GLFWWindow::SetPositionCentered()
{
    int32_t MonitorXpos = 0;
    int32_t MonitorYpos = 0;

    GLFWmonitor* PrimaryMonitor = glfwGetPrimaryMonitor();
    glfwGetMonitorPos(PrimaryMonitor, &MonitorXpos, &MonitorYpos);

    const GLFWvidmode* videoMode = glfwGetVideoMode(PrimaryMonitor);

    const int32_t NewXPos = MonitorXpos + (videoMode->width - m_WindowSpec.Width) / 2;
    const int32_t NewYPos = MonitorYpos + (videoMode->height - m_WindowSpec.Height) / 2;
    glfwSetWindowPos(m_Window, NewXPos, NewYPos);
}

void GLFWWindow::OnUpdate()
{
    glfwPollEvents();

    if (!IsMinimized())
    {
        auto& Context = GraphicsContext::Get();
        Context.SwapBuffers();
        SetVSync(Renderer::GetSettings().VSync);
    }
}

void GLFWWindow::HandleMinimized()
{
    int32_t Width{0}, Height{0};
    glfwGetFramebufferSize(m_Window, &Width, &Height);
    while (Width == 0 || Height == 0 || !IsRunning())
    {
        glfwWaitEvents();
        glfwGetFramebufferSize(m_Window, &Width, &Height);
    }
}

void GLFWWindow::SetWindowLogo(const std::string_view& filePath)
{
    int32_t Width    = 0;
    int32_t Height   = 0;
    int32_t Channels = 0;
    stbi_uc* Pixels  = ImageUtils::LoadImageFromFile(filePath.data(), &Width, &Height, &Channels, false);

    GLFWimage IconImage = {};
    IconImage.pixels    = Pixels;
    IconImage.width     = Width;
    IconImage.height    = Height;

    glfwSetWindowIcon(m_Window, 1, &IconImage);
    ImageUtils::UnloadImage(Pixels);
}

void GLFWWindow::SetVSync(bool bIsVsync)
{
    if (m_bIsVSync == bIsVsync) return;
    m_bIsVSync = bIsVsync;

    auto& gc = GraphicsContext::Get();
    gc.SetVSync(bIsVsync);
}

void GLFWWindow::SetWindowTitle(const std::string_view& title)
{
    GNT_ASSERT(title.data(), "Window title is not valid!");

    glfwSetWindowTitle(m_Window, title.data());
    m_WindowSpec.Title = title;
}

void GLFWWindow::Shutdown()
{
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

GLFWWindow::~GLFWWindow()
{
    Shutdown();
}

}  // namespace Gauntlet