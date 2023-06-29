#include "EclipsePCH.h"
#include "VulkanContext.h"

#include "VulkanUtility.h"

#include "VulkanDevice.h"
#include "VulkanAllocator.h"
#include "VulkanSwapchain.h"
#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"
#include "VulkanRenderPass.h"

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
    ELS_ASSERT(!s_Context, "Graphics context already initialized!");
    s_Context = this;

    CreateInstance();
    CreateDebugMessenger();
    CreateSurface();

    m_Device.reset(new VulkanDevice(m_Instance, m_Surface));
    m_Allocator.reset(new VulkanAllocator(m_Instance, m_Device));
    m_Swapchain.reset(new VulkanSwapchain(m_Device, m_Surface));

    {
        CommandPoolSpecification CommandPoolSpec = {};
        CommandPoolSpec.CommandPoolUsage = ECommandPoolUsage::COMMAND_POOL_DEFAULT_USAGE;
        CommandPoolSpec.QueueFamilyIndex = m_Device->GetQueueFamilyIndices().GetGraphicsFamily();

        m_GraphicsCommandPool.reset(new VulkanCommandPool(CommandPoolSpec));
    }

    {
        CommandPoolSpecification CommandPoolSpec = {};
        CommandPoolSpec.CommandPoolUsage = ECommandPoolUsage::COMMAND_POOL_TRANSFER_USAGE;
        CommandPoolSpec.QueueFamilyIndex = m_Device->GetQueueFamilyIndices().GetTransferFamily();

        m_TransferCommandPool.reset(new VulkanCommandPool(CommandPoolSpec));
    }

    CreateSyncObjects();
    CreateGlobalRenderPass();
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
    DebugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    DebugMessengerCreateInfo.pfnUserCallback = &DebugCallback;

    const auto result = vkCreateDebugUtilsMessengerEXT(m_Instance, &DebugMessengerCreateInfo, nullptr, &m_DebugMessenger);
    ELS_ASSERT(result == VK_SUCCESS, "Failed to create debug messenger!");
}

void VulkanContext::CreateSurface()
{
    /* Some thoughts about surface creation dependent on OS
    * Well maybe I shouldn't float bout "pedantic" surface creation because GLFW is cross-platform && handles such vkCreateXxxSurfaceKHR.
#ifdef ELS_PLATFORM_WINDOWS
    VkWin32SurfaceCreateInfoKHR Win32SurfaceCreateInfo = {};
    Win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    Win32SurfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
    Win32SurfaceCreateInfo.hwnd = glfwGetWin32Window((GLFWwindow*)m_Window->GetNativeWindow());

    const auto result = vkCreateWin32SurfaceKHR(m_Instance, &Win32SurfaceCreateInfo, nullptr, &m_Surface);
    ELS_ASSERT(result == VK_SUCCESS, "Failed to create window surface!");
#else
#endif
*/
    const auto result = glfwCreateWindowSurface(m_Instance, (GLFWwindow*)m_Window->GetNativeWindow(), nullptr, &m_Surface);
    ELS_ASSERT(result == VK_SUCCESS, "Failed to create window surface!");
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

void VulkanContext::CreateSyncObjects()
{
    VkSemaphoreCreateInfo SemaphoreCreateInfo = {};
    SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo FenceCreateInfo = {};
    FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vkCreateFence(m_Device->GetLogicalDevice(), &FenceCreateInfo, nullptr, &m_InFlightFence);
    vkCreateSemaphore(m_Device->GetLogicalDevice(), &SemaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphore);
    vkCreateSemaphore(m_Device->GetLogicalDevice(), &SemaphoreCreateInfo, nullptr, &m_ImageAcquiredSemaphore);
}

void VulkanContext::CreateGlobalRenderPass()
{

    std::vector<VkFormat> Formats{VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    const VkFormat DepthFormat = ChooseSupportedImageFormat(m_Device->GetPhysicalDevice(), Formats, VK_IMAGE_TILING_OPTIMAL,
                                                            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    RenderPassSpecification RenderPassSpec = {};
    {
        std::vector<VkAttachmentDescription> Attachments;
        Attachments.resize(1);
        Attachments[0].format = m_Swapchain->GetImageFormat();
        Attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        // I don't care what you do with the image memory when you load it for use.
        Attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        // Just store the image when you go to store it.
        Attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        // I don't care what the initial layout of the image is.
        Attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        // It better be ready to present to the user when we're done with the renderpass.
        Attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        // We don't care about stencil
        Attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        Attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        /* Attachments[1].format = DepthFormat;
         Attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
         Attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
         Attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
         Attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
         Attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
         Attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
         Attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;*/
        RenderPassSpec.Attachments = Attachments;

        std::vector<VkAttachmentReference> AttachmentRefs;
        AttachmentRefs.resize(1);
        AttachmentRefs[0].attachment = 0;
        AttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        /*AttachmentRefs[1].attachment = 1;
        AttachmentRefs[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;*/
        RenderPassSpec.AttachmentRefs = AttachmentRefs;

        std::vector<VkSubpassDescription> Subpasses;
        Subpasses.resize(1);
        Subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        Subpasses[0].colorAttachmentCount = 1;
        Subpasses[0].pColorAttachments = &AttachmentRefs[0];
        // Subpasses[0].pDepthStencilAttachment = &AttachmentRefs[1];
        RenderPassSpec.Subpasses = Subpasses;

        // RenderPassSpec.DepthImageView = DepthImageView;
        m_GlobalRenderPass.reset(new VulkanRenderPass(RenderPassSpec));
    }
}

void VulkanContext::RecreateSwapchain()
{
    //m_Window->HandleMinimized();
    m_Device->WaitDeviceOnFinish();

    LOG_WARN("Swapchain recreation!");

    m_Swapchain->Destroy();
    m_GlobalRenderPass->Destroy();

    m_Swapchain->Create();
    CreateGlobalRenderPass();
}

void VulkanContext::BeginRender()
{
    // Firstly wait for GPU to finish drawing therefore set fence to signaled.
    VK_CHECK(vkWaitForFences(m_Device->GetLogicalDevice(), 1, &m_InFlightFence, VK_TRUE, UINT64_MAX), "Failed to wait for fences!");

    const auto Result = m_Swapchain->TryAcquireNextImage(m_ImageAcquiredSemaphore);
    if (!Result)
    {
        RecreateSwapchain();  // Recreate and return, next cycle will be correct image extent.
        return;
    }
    // Next set fence state to unsignaled.
    VK_CHECK(vkResetFences(m_Device->GetLogicalDevice(), 1, &m_InFlightFence), "Failed to reset fences!");

    auto CommandBuffer = m_GraphicsCommandPool->GetCommandBuffer(0);
    CommandBuffer.BeginRecording();

    std::vector<VkClearValue> ClearValues = {{{0.0f, 1.0f, 1.0f, 1.0f}}};
    m_GlobalRenderPass->Begin(CommandBuffer.Get(), ClearValues);
}

void VulkanContext::EndRender()
{
    auto CommandBuffer = m_GraphicsCommandPool->GetCommandBuffer(0);
    m_GlobalRenderPass->End(CommandBuffer.Get());

    CommandBuffer.EndRecording();

    std::vector<VkPipelineStageFlags> WaitStages{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &CommandBuffer.Get();
    SubmitInfo.waitSemaphoreCount = 1;
    SubmitInfo.pWaitSemaphores = &m_ImageAcquiredSemaphore;  // Wait for semaphore signal until we acuired image from the swapchain
    SubmitInfo.signalSemaphoreCount = 1;
    SubmitInfo.pSignalSemaphores = &m_RenderFinishedSemaphore;  // Signal semaphore when render finished
    SubmitInfo.pWaitDstStageMask = WaitStages.data();

    // Submit command buffer to the queue and execute it.
    // RenderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &SubmitInfo, m_InFlightFence), "Failed to submit command buffes to the queue.");
}

void VulkanContext::SwapBuffers()
{
    const auto bIsPresentedSuccessfully = m_Swapchain->TryPresentImage(m_RenderFinishedSemaphore);
    if (!bIsPresentedSuccessfully)
    {
        RecreateSwapchain();
    }
}

void VulkanContext::SetVSync(bool IsVSync) {}

void VulkanContext::Destroy()
{
    m_Device->WaitDeviceOnFinish();

    m_GlobalRenderPass->Destroy();

    vkDestroyFence(m_Device->GetLogicalDevice(), m_InFlightFence, nullptr);
    vkDestroySemaphore(m_Device->GetLogicalDevice(), m_RenderFinishedSemaphore, nullptr);
    vkDestroySemaphore(m_Device->GetLogicalDevice(), m_ImageAcquiredSemaphore, nullptr);

    m_TransferCommandPool->Destroy();
    m_GraphicsCommandPool->Destroy();

    m_Swapchain->Destroy();
    m_Allocator->Destroy();
    m_Device->Destroy();

    if (bEnableValidationLayers)
    {
        vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    vkDestroyInstance(m_Instance, nullptr);
    s_Context = nullptr;
    LOG_INFO("Vulkan context destroyed!");
}

}  // namespace Eclipse