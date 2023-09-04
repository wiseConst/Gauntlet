#include "GauntletPCH.h"
#include "VulkanContext.h"

#include "VulkanUtility.h"

#include "VulkanDevice.h"
#include "VulkanAllocator.h"
#include "VulkanSwapchain.h"
#include "VulkanCommandPool.h"
#include "VulkanDescriptors.h"

#include "Gauntlet/Core/Application.h"
#include "Gauntlet/Core/JobSystem.h"
#include "Gauntlet/Core/Window.h"
#include "Gauntlet/Renderer/Renderer.h"

namespace Gauntlet
{

VulkanContext::VulkanContext(Scoped<Window>& window) : GraphicsContext(window)
{
    GNT_ASSERT(!s_Context, "Graphics context already initialized!");
    s_Context = this;

    CreateInstance();
    CreateDebugMessenger();
    CreateSurface();

    m_Device.reset(new VulkanDevice(m_Instance, m_Surface));
    m_Allocator.reset(new VulkanAllocator(m_Instance, m_Device));

    m_Swapchain.reset(new VulkanSwapchain(m_Device, m_Surface));

    {
        CommandPoolSpecification CommandPoolSpec = {};
        CommandPoolSpec.CommandPoolUsage         = ECommandPoolUsage::COMMAND_POOL_DEFAULT_USAGE;
        CommandPoolSpec.QueueFamilyIndex         = m_Device->GetQueueFamilyIndices().GraphicsFamily;

        m_GraphicsCommandPool.reset(new VulkanCommandPool(CommandPoolSpec));
    }

    {
        CommandPoolSpecification CommandPoolSpec = {};
        CommandPoolSpec.CommandPoolUsage         = ECommandPoolUsage::COMMAND_POOL_TRANSFER_USAGE;
        CommandPoolSpec.QueueFamilyIndex         = m_Device->GetQueueFamilyIndices().TransferFamily;

        m_TransferCommandPool.reset(new VulkanCommandPool(CommandPoolSpec));
    }

    CreateSyncObjects();

    m_DescriptorAllocator.reset(new VulkanDescriptorAllocator(m_Device));

    InitializeMultithreadedRenderer();
}

void VulkanContext::CreateInstance()
{
    {
        const auto result = volkInitialize();
        GNT_ASSERT(result == VK_SUCCESS, "Failed to initialize volk( meta-loader for Vulkan )!");
    }

    GNT_ASSERT(CheckVulkanAPISupport() == true, "Vulkan is not supported!");

    if (s_bEnableValidationLayers)
    {
        GNT_ASSERT(CheckValidationLayerSupport() == true, "Validation layers requested but not supported!");
    }

#if GNT_DEBUG && LOG_VULKAN_INFO
    {
        uint32_t SupportedApiVersionFromDLL;
        const auto result = vkEnumerateInstanceVersion(&SupportedApiVersionFromDLL);
        GNT_ASSERT(result == VK_SUCCESS, "Failed to retrieve supported vulkan version!");

        LOG_INFO("Your system supports vulkan version up to: %u.%u.%u.%u", VK_API_VERSION_VARIANT(SupportedApiVersionFromDLL),
                 VK_API_VERSION_MAJOR(SupportedApiVersionFromDLL), VK_API_VERSION_MINOR(SupportedApiVersionFromDLL),
                 VK_API_VERSION_PATCH(SupportedApiVersionFromDLL));
    }
#endif

    const auto& app                    = Application::Get();
    VkApplicationInfo ApplicationInfo  = {};
    ApplicationInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ApplicationInfo.apiVersion         = GNT_VK_API_VERSION;
    ApplicationInfo.applicationVersion = ApplicationVersion;
    ApplicationInfo.engineVersion      = EngineVersion;
    ApplicationInfo.pApplicationName   = app.GetSpecification().AppName.data();
    ApplicationInfo.pEngineName        = EngineName;

    const auto RequiredExtensions              = GetRequiredExtensions();
    VkInstanceCreateInfo InstanceCreateInfo    = {};
    InstanceCreateInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    InstanceCreateInfo.pApplicationInfo        = &ApplicationInfo;
    InstanceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(RequiredExtensions.size());
    InstanceCreateInfo.ppEnabledExtensionNames = RequiredExtensions.data();

#if GNT_DEBUG
    InstanceCreateInfo.enabledLayerCount   = static_cast<uint32_t>(VulkanLayers.size());
    InstanceCreateInfo.ppEnabledLayerNames = VulkanLayers.data();
#else
    InstanceCreateInfo.enabledLayerCount   = 0;
    InstanceCreateInfo.ppEnabledLayerNames = nullptr;
#endif

    const auto result = vkCreateInstance(&InstanceCreateInfo, nullptr, &m_Instance);
    GNT_ASSERT(result == VK_SUCCESS, "Failed to create instance!");

    volkLoadInstanceOnly(m_Instance);
}

void VulkanContext::CreateDebugMessenger()
{
    if (!s_bEnableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo = {};
    DebugMessengerCreateInfo.sType                              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    DebugMessengerCreateInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    DebugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    DebugMessengerCreateInfo.pfnUserCallback = &DebugCallback;

    const auto result = vkCreateDebugUtilsMessengerEXT(m_Instance, &DebugMessengerCreateInfo, nullptr, &m_DebugMessenger);
    GNT_ASSERT(result == VK_SUCCESS, "Failed to create debug messenger!");
}

void VulkanContext::CreateSurface()
{
    const auto result = glfwCreateWindowSurface(m_Instance, (GLFWwindow*)m_Window->GetNativeWindow(), nullptr, &m_Surface);
    GNT_ASSERT(result == VK_SUCCESS, "Failed to create window surface!");
}

bool VulkanContext::CheckVulkanAPISupport() const
{
    uint32_t ExtensionCount = 0;
    {
        const auto result = vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr);
        GNT_ASSERT(result == VK_SUCCESS && ExtensionCount != 0, "Can't retrieve number of supported extensions.");
    }

    std::vector<VkExtensionProperties> Extensions(ExtensionCount);
    {
        const auto result = vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, Extensions.data());
        GNT_ASSERT(result == VK_SUCCESS && ExtensionCount != 0, "Can't retrieve supported extensions.");
    }

#if LOG_VULKAN_INFO && GNT_DEBUG
    LOG_INFO("List of supported vulkan extensions. (%u)", ExtensionCount);
    for (const auto& Ext : Extensions)
    {
        LOG_TRACE(" %s, version: %u.%u.%u", Ext.extensionName, VK_API_VERSION_MAJOR(Ext.specVersion), VK_API_VERSION_MINOR(Ext.specVersion),
                  VK_API_VERSION_PATCH(Ext.specVersion));
    }
#endif

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    GNT_ASSERT(glfwExtensionCount != 0, "glfwExtensions are empty!");

#if GNT_DEBUG && LOG_VULKAN_INFO
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
        GNT_ASSERT(result == VK_SUCCESS && LayerCount != 0, "Can't retrieve number of supported validation layers.");
    }

    std::vector<VkLayerProperties> AvailableLayers(LayerCount);
    {
        const auto result = vkEnumerateInstanceLayerProperties(&LayerCount, AvailableLayers.data());
        GNT_ASSERT(result == VK_SUCCESS && LayerCount != 0, "Can't retrieve number of supported validation layers.");
    }

#if GNT_DEBUG && LOG_VULKAN_INFO
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
    GNT_ASSERT(glfwExtensionCount != 0, "Failed to retrieve glfwExtensions");

    std::vector<const char*> Extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (s_bEnableValidationLayers)
    {
        Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return Extensions;
}

void VulkanContext::CreateSyncObjects()
{
    VkSemaphoreCreateInfo SemaphoreCreateInfo = {};
    SemaphoreCreateInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo FenceCreateInfo = {};
    FenceCreateInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    FenceCreateInfo.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

    m_InFlightFences.resize(FRAMES_IN_FLIGHT);
    m_RenderFinishedSemaphores.resize(FRAMES_IN_FLIGHT);
    m_ImageAcquiredSemaphores.resize(FRAMES_IN_FLIGHT);

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        vkCreateFence(m_Device->GetLogicalDevice(), &FenceCreateInfo, nullptr, &m_InFlightFences[i]);
        vkCreateSemaphore(m_Device->GetLogicalDevice(), &SemaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphores[i]);
        vkCreateSemaphore(m_Device->GetLogicalDevice(), &SemaphoreCreateInfo, nullptr, &m_ImageAcquiredSemaphores[i]);
    }
}

void VulkanContext::BeginRender()
{
    // Firstly wait for GPU to finish drawing therefore set fence to signaled.
    VK_CHECK(vkWaitForFences(m_Device->GetLogicalDevice(), 1, &m_InFlightFences[m_Swapchain->GetCurrentFrameIndex()], VK_TRUE, UINT64_MAX),
             "Failed to wait for fences!");

    const auto CurrentTime           = Application::GetTimeNow();
    Renderer::GetStats().CPUWaitTime = CurrentTime - m_CPULastWaitTime;

    m_GPULastWaitTime = CurrentTime;

    if (!m_Swapchain->TryAcquireNextImage(m_ImageAcquiredSemaphores[m_Swapchain->GetCurrentFrameIndex()])) return;

    // Reset fence if only we've acquired an image, otherwise if you reset it after waiting && swapchain recreated then here will be
    // deadlock because fence is unsignaled, but we will be waiting it to be signaled.
    VK_CHECK(vkResetFences(m_Device->GetLogicalDevice(), 1, &m_InFlightFences[m_Swapchain->GetCurrentFrameIndex()]),
             "Failed to reset fences!");

    m_CurrentCommandBuffer = &m_GraphicsCommandPool->GetCommandBuffer(m_Swapchain->GetCurrentFrameIndex());
    GNT_ASSERT(m_CurrentCommandBuffer, "Failed to retrieve graphics command buffer!");

    m_CurrentCommandBuffer->BeginRecording();
}

void VulkanContext::EndRender()
{
    m_CurrentCommandBuffer->EndRecording();

    std::vector<VkPipelineStageFlags> WaitStages{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo SubmitInfo       = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers    = &m_CurrentCommandBuffer->Get();
    SubmitInfo.waitSemaphoreCount = 1;
    SubmitInfo.pWaitSemaphores    = &m_ImageAcquiredSemaphores[m_Swapchain->GetCurrentFrameIndex()];  // Wait for semaphore signal until we
                                                                                                      // acquired image from the swapchain
    SubmitInfo.signalSemaphoreCount = 1;
    SubmitInfo.pSignalSemaphores =
        &m_RenderFinishedSemaphores[m_Swapchain->GetCurrentFrameIndex()];  // Signal semaphore when render finished
    SubmitInfo.pWaitDstStageMask = WaitStages.data();

    const auto CurrentTime           = Application::GetTimeNow();
    Renderer::GetStats().GPUWaitTime = CurrentTime - m_CPULastWaitTime;

    m_CPULastWaitTime = CurrentTime;
    // InFlightFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &SubmitInfo, m_InFlightFences[m_Swapchain->GetCurrentFrameIndex()]),
             "Failed to submit command buffes to the queue.");
}

void VulkanContext::SwapBuffers()
{
    m_Swapchain->PresentImage(m_RenderFinishedSemaphores[m_Swapchain->GetCurrentFrameIndex()]);
}

void VulkanContext::SetVSync(bool bIsVSync)
{
    m_Device->WaitDeviceOnFinish();

    float SwapchainRecreationStartTime = Application::Get().GetTimeNow();
    m_Swapchain->Invalidate();
    float SwapchainRecreationEndTime = Application::Get().GetTimeNow();
    LOG_INFO("Time took to set vsync(invalidate swapchain): (%0.2f)ms",
             (SwapchainRecreationEndTime - SwapchainRecreationStartTime) * 1000.0f);
}

void VulkanContext::Destroy()
{
    m_Device->WaitDeviceOnFinish();

    // Multithreaded stuff
    for (auto& threadData : m_ThreadData)
    {
        threadData.CommandPool->Destroy();
    }

    m_CurrentCommandBuffer = nullptr;
    m_DescriptorAllocator->Destroy();
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroyFence(m_Device->GetLogicalDevice(), m_InFlightFences[i], nullptr);
        vkDestroySemaphore(m_Device->GetLogicalDevice(), m_RenderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_Device->GetLogicalDevice(), m_ImageAcquiredSemaphores[i], nullptr);
    }

    m_TransferCommandPool->Destroy();
    m_GraphicsCommandPool->Destroy();

    m_Swapchain->Destroy();
    m_Allocator->Destroy();
    m_Device->Destroy();

    if (s_bEnableValidationLayers)
    {
        vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    vkDestroyInstance(m_Instance, nullptr);

#if GNT_DEBUG
    LOG_INFO("Vulkan context destroyed!");
#endif
}

void VulkanContext::AddSwapchainResizeCallback(const std::function<void()>& resizeCallback)
{
    m_Swapchain->AddResizeCallback(resizeCallback);
}

void VulkanContext::InitializeMultithreadedRenderer()
{
    m_ThreadData.resize(JobSystem::GetThreadCount());
    for (uint32_t i = 0; i < m_ThreadData.size(); ++i)
    {
        CommandPoolSpecification CommandPoolSpec = {};
        CommandPoolSpec.CommandPoolUsage         = ECommandPoolUsage::COMMAND_POOL_DEFAULT_USAGE;
        CommandPoolSpec.QueueFamilyIndex         = m_Device->GetQueueFamilyIndices().GraphicsFamily;

        m_ThreadData[i].CommandPool = Scoped<VulkanCommandPool>(new VulkanCommandPool(CommandPoolSpec));

        m_ThreadData[i].SecondaryShadowMapCommandBuffers.resize(FRAMES_IN_FLIGHT);
        m_ThreadData[i].CommandPool->AllocateSecondaryCommandBuffers(m_ThreadData[i].SecondaryShadowMapCommandBuffers);

        m_ThreadData[i].SecondaryGeometryCommandBuffers.resize(FRAMES_IN_FLIGHT);
        m_ThreadData[i].CommandPool->AllocateSecondaryCommandBuffers(m_ThreadData[i].SecondaryGeometryCommandBuffers);
    }
}

}  // namespace Gauntlet