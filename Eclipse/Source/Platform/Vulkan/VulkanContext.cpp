#include "EclipsePCH.h"
#include "VulkanContext.h"

#include "VulkanDevice.h"
#include "VulkanUtility.h"

#include "Eclipse/Core/Application.h"
#include "Eclipse/Core/Window.h"

#ifdef ELS_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace Eclipse
{
VulkanContext::VulkanContext(Scoped<Window>& InWindow) : GraphicsContext(InWindow)
{
    CreateInstance();
    CreateDebugMessenger();
    CreateSurface();

    m_Device.reset(new VulkanDevice(m_Instance, m_Surface));
}

void VulkanContext::CreateInstance()
{
    {
        const auto result = volkInitialize();
        ELS_ASSERT(result == VK_SUCCESS, "Failed to initialize volk( meta-loader for Vulkan )!");
    }

    ELS_ASSERT(CheckVulkanAPISupport() == true, "Vulkan is not supported!");

    if (bEnableValidationLayers)
    {
        ELS_ASSERT(CheckValidationLayerSupport() == true, "Validation layers requested but not supported!");
    }

    {
        uint32_t SupportedApiVersion;
        const auto result = vkEnumerateInstanceVersion(&SupportedApiVersion);
        ELS_ASSERT(result == VK_SUCCESS, "Failed to retrieve supported vulkan version!");

        LOG_INFO("Your system supports vulkan version up to: %u.%u.%u.%u", VK_API_VERSION_VARIANT(SupportedApiVersion),
                 VK_API_VERSION_MAJOR(SupportedApiVersion), VK_API_VERSION_MINOR(SupportedApiVersion),
                 VK_API_VERSION_PATCH(SupportedApiVersion));
    }

    const auto& app = Application::Get();
    VkApplicationInfo ApplicationInfo = {};
    ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ApplicationInfo.apiVersion = ELS_VK_API_VERSION;
    ApplicationInfo.applicationVersion = ApplicationVersion;
    ApplicationInfo.engineVersion = EngineVersion;
    ApplicationInfo.pApplicationName = app.GetInfo().AppName.data();
    ApplicationInfo.pEngineName = EngineName;

    const auto RequiredExtensions = GetRequiredExtensions();
    VkInstanceCreateInfo InstanceCreateInfo = {};
    InstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    InstanceCreateInfo.pApplicationInfo = &ApplicationInfo;
    InstanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(RequiredExtensions.size());
    InstanceCreateInfo.ppEnabledExtensionNames = RequiredExtensions.data();

#if ELS_DEBUG
    InstanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VulkanLayers.size());
    InstanceCreateInfo.ppEnabledLayerNames = VulkanLayers.data();
#else
    InstanceCreateInfo.enabledLayerCount = 0;
    InstanceCreateInfo.ppEnabledLayerNames = nullptr;
#endif

    const auto result = vkCreateInstance(&InstanceCreateInfo, nullptr, &m_Instance);
    ELS_ASSERT(result == VK_SUCCESS, "Failed to create instance!");

    volkLoadInstanceOnly(m_Instance);
}

void VulkanContext::CreateDebugMessenger()
{
    if (!bEnableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo = {};
    DebugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
     DebugMessengerCreateInfo.messageSeverity =
         VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    DebugMessengerCreateInfo.messageType= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
     DebugMessengerCreateInfo.pfnUserCallback = &DebugCallback;


    const auto result = vkCreateDebugUtilsMessengerEXT(m_Instance, &DebugMessengerCreateInfo, nullptr, &m_DebugMessenger);
    ELS_ASSERT(result == VK_SUCCESS, "Failed to create debug messenger!");
}

void VulkanContext::CreateSurface()
{
    // TODO: Add linux glfwGetX11Window(window);   
#ifdef ELS_PLATFORM_WINDOWS
    VkWin32SurfaceCreateInfoKHR Win32SurfaceCreateInfo = {};
    Win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    Win32SurfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
    Win32SurfaceCreateInfo.hwnd = glfwGetWin32Window((GLFWwindow*)m_Window->GetNativeWindow());

    const auto result = vkCreateWin32SurfaceKHR(m_Instance, &Win32SurfaceCreateInfo, nullptr, &m_Surface);
    ELS_ASSERT(result == VK_SUCCESS, "Failed to create window surface!");
#else
    const auto result = glfwCreateWindowSurface(m_Instance, (GLFWwindow*)m_Window->GetNativeWindow(), nullptr, &m_Surface);
    ELS_ASSERT(result == VK_SUCCESS, "Failed to create window surface!");
#endif
}

bool VulkanContext::CheckVulkanAPISupport() const
{
    uint32_t ExtensionCount = 0;
    {
        const auto result = vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr);
        ELS_ASSERT(result == VK_SUCCESS && ExtensionCount != 0, "Can't retrieve number of supported extensions.")
    }

    std::vector<VkExtensionProperties> Extensions(ExtensionCount);
    {
        const auto result = vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, Extensions.data());
        ELS_ASSERT(result == VK_SUCCESS && ExtensionCount != 0, "Can't retrieve supported extensions.")
    }

#if LOG_VULKAN_INFO && ELS_DEBUG
    LOG_INFO("List of supported vulkan extensions. (%u)", ExtensionCount);
    for (const auto& Ext : Extensions)
    {
        LOG_TRACE(" %s, version: %u.%u.%u", Ext.extensionName, VK_API_VERSION_MAJOR(Ext.specVersion), VK_API_VERSION_MINOR(Ext.specVersion),
                  VK_API_VERSION_PATCH(Ext.specVersion));
    }
#endif

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    ELS_ASSERT(glfwExtensionCount != 0, "glfwExtensions are empty!");

#if ELS_DEBUG && LOG_VULKAN_INFO
    LOG_INFO("List of extensions required by GLFW. (%u)", glfwExtensionCount);

    for (uint32_t i = 0; i < glfwExtensionCount; ++i)
    {
        LOG_TRACE(" %s", glfwExtensions[i]);
    }
#endif

    // Check if current vulkan version supports required by GLFW extensions
    for (uint32_t i = 0; i < glfwExtensionCount; ++i)
    {
        bool bIsSupported = false;
        for (const auto& Ext : Extensions)
        {
            if (strcmp(Ext.extensionName, glfwExtensions[i]) == 0)
            {
                bIsSupported = true;
                break;
            }
        }

        if (!bIsSupported)
        {
            return false;
        }
    }

    return glfwVulkanSupported();
}

bool VulkanContext::CheckValidationLayerSupport()
{
    uint32_t LayerCount{0};
    {
        const auto result = vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);
        ELS_ASSERT(result == VK_SUCCESS && LayerCount != 0, "Can't retrieve number of supported validation layers.")
    }

    std::vector<VkLayerProperties> AvailableLayers(LayerCount);
    {
        const auto result = vkEnumerateInstanceLayerProperties(&LayerCount, AvailableLayers.data());
        ELS_ASSERT(result == VK_SUCCESS && LayerCount != 0, "Can't retrieve number of supported validation layers.")
    }

#if ELS_DEBUG && LOG_VULKAN_INFO
    LOG_INFO("List of validation layers: (%u)", LayerCount);

    for (const auto& Layer : AvailableLayers)
    {
        LOG_TRACE(" %s", Layer.layerName);
    }
#endif

    for (const char* ValidationLayerName : VulkanLayers)
    {
        bool bIslayerFound = false;

        for (const auto& LayerProps : AvailableLayers)
        {
            if (strcmp(ValidationLayerName, LayerProps.layerName) == 0)
            {
                bIslayerFound = true;
                break;
            }
        }

        if (!bIslayerFound)
        {
            return false;
        }
    }

    return true;
}

const std::vector<const char*> VulkanContext::GetRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    ELS_ASSERT(glfwExtensionCount != 0, "Failed to retrieve glfwExtensions");

    std::vector<const char*> Extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (bEnableValidationLayers)
    {
        Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return Extensions;
}

void VulkanContext::BeginRender() {
    //auto& app = Application::Get();
    //app.SubmitToRenderThread([]() { LOG_WARN("Render Thread"); });
}

void VulkanContext::EndRender() {}

void VulkanContext::Destroy()
{
    m_Device->WaitDeviceOnFinish();

    m_Device->Destroy();

    if (bEnableValidationLayers)
    {
        vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    vkDestroyInstance(m_Instance, nullptr);
}

}  // namespace Eclipse