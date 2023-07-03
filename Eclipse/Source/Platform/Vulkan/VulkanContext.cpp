#include "EclipsePCH.h"
#include "VulkanContext.h"

#include "VulkanUtility.h"

#include "VulkanDevice.h"
#include "VulkanAllocator.h"
#include "VulkanSwapchain.h"

#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"

#include "VulkanRenderPass.h"
#include "VulkanShader.h"
#include "VulkanPipeline.h"
#include "VulkanBuffer.h"
#include "VulkanMesh.h"
#include "VulkanTexture.h"

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

    ImageSpecification DepthImageSpec = {};
    DepthImageSpec.Format = VK_FORMAT_D32_SFLOAT;
    DepthImageSpec.Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    DepthImageSpec.ImageViewAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    DepthImageSpec.ImageViewFormat = VK_FORMAT_D32_SFLOAT;
    m_Swapchain.reset(new VulkanSwapchain(m_Device, m_Surface, DepthImageSpec));

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
    Ref<VulkanShader> VertexShader(new VulkanShader("Resources/Shaders/FlatColor.vert.spv"));
    Ref<VulkanShader> FragmentShader(new VulkanShader("Resources/Shaders/FlatColor.frag.spv"));

    m_MonkeyMesh.reset(new VulkanMesh("Resources/Models/Monkey/monkey_smooth.obj"));

    std::vector<Vertex> VertexData(3);
    VertexData[0].Position = {1.0f, 1.0f, 0.0f};
    VertexData[0].Color = {1.0f, 0.0f, 0.0f};

    VertexData[1].Position = {-1.0f, 1.0f, 0.0f};
    VertexData[1].Color = {0.0f, 1.0f, 0.0f};

    VertexData[2].Position = {0.0f, -1.0f, 0.0f};
    VertexData[2].Color = {0.0f, 0.0f, 1.0f};

    BufferInfo VertexBufferInfo = {};
    VertexBufferInfo.Usage = EBufferUsage::VERTEX_BUFFER;
    VertexBufferInfo.Data = (void*)VertexData.data();
    VertexBufferInfo.Count = VertexData.size();
    VertexBufferInfo.Size = VertexData.size() * sizeof(VertexData[0]);

    BufferLayout VertexBufferLayout = {
        {EShaderDataType::Vec3, "InPosition"},  //
        {EShaderDataType::Vec3, "InNormal"},    //
        {EShaderDataType::Vec3, "InColor"}      //
    };                                          //
    m_TriangleVertexBuffer.reset(new VulkanVertexBuffer(VertexBufferInfo));
    m_TriangleVertexBuffer->SetLayout(VertexBufferLayout);

    {
        PipelineSpecification PipelineSpec = {};
        PipelineSpec.RenderPass = m_GlobalRenderPass->Get();
        PipelineSpec.FrontFace = EFrontFace::FRONT_FACE_CLOCKWISE;
        PipelineSpec.PolygonMode = EPolygonMode::POLYGON_MODE_FILL;
        PipelineSpec.PrimitiveTopology = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        PipelineSpec.CullMode = ECullMode::CULL_MODE_NONE;

        PipelineSpec.ShaderStages.resize(2);
        PipelineSpec.ShaderStages[0].Shader = VertexShader;
        PipelineSpec.ShaderStages[0].Stage = EShaderStage::SHADER_STAGE_VERTEX;

        PipelineSpec.ShaderStages[1].Shader = FragmentShader;
        PipelineSpec.ShaderStages[1].Stage = EShaderStage::SHADER_STAGE_FRAGMENT;

        std::vector<VkVertexInputAttributeDescription> ShaderAttributeDescriptions(VertexBufferLayout.GetElements().size());
        for (uint32_t i = 0; i < ShaderAttributeDescriptions.size(); ++i)
        {
            ShaderAttributeDescriptions[i] = GetShaderAttributeDescription(
                0, EclipseFormatToVulkan(VertexBufferLayout.GetElements()[i].Type), i, VertexBufferLayout.GetElements()[i].Offset);
        }

        PipelineSpec.ShaderAttributeDescriptions = ShaderAttributeDescriptions;
        PipelineSpec.ShaderBindingDescriptions = {GetShaderBindingDescription(0, VertexBufferLayout.GetStride())};

        VkPushConstantRange PushConstantRange = {};
        PushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;  // accessible only in vertex shader
        PushConstantRange.offset = 0;
        PushConstantRange.size = sizeof(MeshPushConstants);

        PipelineSpec.PushConstantRanges = {PushConstantRange};
        PipelineSpec.bDepthTest = VK_TRUE;
        PipelineSpec.bDepthWrite = VK_TRUE;
        PipelineSpec.DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

        m_TrianglePipeline.reset(new VulkanPipeline(PipelineSpec));

        VertexShader->DestroyModule();
        FragmentShader->DestroyModule();
    }

    LOG_TRACE("Mesh Push Constants size: %u", m_TrianglePipeline->GetPushConstantsSize());
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

void VulkanContext::CreateGlobalRenderPass()
{
    RenderPassSpecification RenderPassSpec = {};
    {
        std::vector<VkAttachmentDescription> Attachments(2);
        Attachments[0].format = m_Swapchain->GetImageFormat();
        Attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        Attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        Attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        Attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        Attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        Attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        Attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        Attachments[1].format = m_Swapchain->GetDepthFormat();
        Attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        Attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        Attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        Attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        Attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        Attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        Attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        RenderPassSpec.Attachments = Attachments;

        std::vector<VkAttachmentReference> AttachmentRefs(2);
        AttachmentRefs[0].attachment = 0;
        AttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        AttachmentRefs[1].attachment = 1;
        AttachmentRefs[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        RenderPassSpec.AttachmentRefs = AttachmentRefs;

        std::vector<VkSubpassDescription> Subpasses(1);
        Subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        Subpasses[0].colorAttachmentCount = 1;
        Subpasses[0].pColorAttachments = &AttachmentRefs[0];
        Subpasses[0].pDepthStencilAttachment = &AttachmentRefs[1];
        RenderPassSpec.Subpasses = Subpasses;

        /* Subpass Dependency
         * Now we have to adjust the renderpass synchronization.
         * Previously, it was possible that multiple frames were rendered simultaneously by the GPU.
         * This is a problem when using depth buffers, because one frame could overwrite the depth buffer
         * while a previous frame is still rendering to it.
         */
        std::vector<VkSubpassDependency> Dependencies(2);
        // We keep the subpass dependency for the color attachment we were already using:
        Dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        Dependencies[0].dstSubpass = 0;
        Dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;  // before bottom pipe bit
        Dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        Dependencies[0].srcAccessMask = 0;
        Dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // We also add a new subpass dependency that synchronizes accesses to depth attachments.
        Dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
        Dependencies[1].dstSubpass = 0;
        Dependencies[1].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        Dependencies[1].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        Dependencies[1].srcAccessMask = 0;
        Dependencies[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        // This dependency tells Vulkan that the depth attachment in a renderpass cannot be used before previous renderpasses have finished
        // using it.

        RenderPassSpec.Dependencies = Dependencies;

        RenderPassSpec.DepthImageView = m_Swapchain->GetDepthImageView();
        m_GlobalRenderPass.reset(new VulkanRenderPass(RenderPassSpec));
    }
}

void VulkanContext::RecreateSwapchain()
{
    if (m_Window->IsMinimized()) return;  // m_Window->HandleMinimized(); // Blocking loop
    m_Device->WaitDeviceOnFinish();

    m_GlobalRenderPass->Destroy();
    m_Swapchain->Destroy();

    m_Swapchain->Create();
    CreateGlobalRenderPass();

    LOG_WARN("Swapchain recreated, new window size: (%u, %u).", m_Swapchain->GetImageExtent().width, m_Swapchain->GetImageExtent().height);
}

void VulkanContext::BeginRender()
{
    m_StartCPUWaitTime = (float)glfwGetTime();

    // Firstly wait for GPU to finish drawing therefore set fence to signaled.
    VK_CHECK(vkWaitForFences(m_Device->GetLogicalDevice(), 1, &m_InFlightFences[m_Swapchain->GetCurrentFrameIndex()], VK_TRUE, UINT64_MAX),
             "Failed to wait for fences!");

    const auto bIsAcquired = m_Swapchain->TryAcquireNextImage(m_ImageAcquiredSemaphores[m_Swapchain->GetCurrentFrameIndex()]);
    if (!bIsAcquired)
    {
        RecreateSwapchain();  // Recreate and return, next cycle will be correct image extent.
        return;
    }
    // Next set fence state to unsignaled.
    VK_CHECK(vkResetFences(m_Device->GetLogicalDevice(), 1, &m_InFlightFences[m_Swapchain->GetCurrentFrameIndex()]),
             "Failed to reset fences!");

    auto& CommandBuffer = m_GraphicsCommandPool->GetCommandBuffer(m_Swapchain->GetCurrentFrameIndex());
    CommandBuffer.BeginRecording();

    std::vector<VkClearValue> ClearValues(2);
    ClearValues[0].color = {0.0f, 1.0f, 1.0f, 1.0f};
    ClearValues[1].depthStencil.depth = 1.0f;

    m_GlobalRenderPass->Begin(CommandBuffer.Get(), ClearValues);

    CommandBuffer.SetViewportAndScissors();
    CommandBuffer.BindPipeline(m_TrianglePipeline->Get());

    VkDeviceSize Offset = 0;
    CommandBuffer.BindVertexBuffers(0, 1, &m_MonkeyMesh->GetVertexBuffer<VulkanVertexBuffer>()->Get(), &Offset);

    // View matrix
    glm::vec3 CameraPos = {0.f, 0.f, -5.f};
    glm::mat4 View = glm::translate(glm::mat4(1.f), CameraPos);

    // Projection matrix
    glm::mat4 Projection = glm::perspective(
        glm::radians(90.0f), (float)m_Swapchain->GetImageExtent().width / (float)m_Swapchain->GetImageExtent().height, 0.001f, 100.0f);
    Projection[1][1] *= -1;

    // Model matrix
    glm::mat4 Model = glm::rotate(glm::mat4(1.0f), glm::radians(m_Swapchain->GetCurrentFrameIndex() * 0.4f), glm::vec3(0, 1, 0));
    MeshPushConstants PushConstants = {};
    PushConstants.RenderMatrix = Projection * View * Model;

    // Upload the matrix to the GPU via push constants
    CommandBuffer.BindPushConstants(m_TrianglePipeline->GetLayout(), m_TrianglePipeline->GetPushConstantsShaderStageFlags(), 0,
                                    m_TrianglePipeline->GetPushConstantsSize(), &PushConstants);
    CommandBuffer.Draw(m_MonkeyMesh->GetVertexBuffer()->GetCount() /*m_TriangleVertexBuffer->GetCount()*/);
}

void VulkanContext::EndRender()
{
    auto& CommandBuffer = m_GraphicsCommandPool->GetCommandBuffer(m_Swapchain->GetCurrentFrameIndex());
    m_GlobalRenderPass->End(CommandBuffer.Get());

    CommandBuffer.EndRecording();

    std::vector<VkPipelineStageFlags> WaitStages{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &CommandBuffer.Get();
    SubmitInfo.waitSemaphoreCount = 1;
    SubmitInfo.pWaitSemaphores = &m_ImageAcquiredSemaphores[m_Swapchain->GetCurrentFrameIndex()];  // Wait for semaphore signal until we
                                                                                                   // acuired image from the swapchain
    SubmitInfo.signalSemaphoreCount = 1;
    SubmitInfo.pSignalSemaphores =
        &m_RenderFinishedSemaphores[m_Swapchain->GetCurrentFrameIndex()];  // Signal semaphore when render finished
    SubmitInfo.pWaitDstStageMask = WaitStages.data();

    // Submit command buffer to the queue and execute it.
    // InFlightFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &SubmitInfo, m_InFlightFences[m_Swapchain->GetCurrentFrameIndex()]),
             "Failed to submit command buffes to the queue.");

    // CPU wait time calculation
    const float CpuWaitTime = (float)glfwGetTime() - m_StartCPUWaitTime;
    s_RenderStats.CPUWaitTime = CpuWaitTime * 1000.0f;
}

void VulkanContext::SwapBuffers()
{
    const auto bIsPresentedSuccessfully = m_Swapchain->TryPresentImage(m_RenderFinishedSemaphores[m_Swapchain->GetCurrentFrameIndex()]);
    if (!bIsPresentedSuccessfully)
    {
        RecreateSwapchain();
    }
}

void VulkanContext::SetVSync(bool IsVSync)
{
    m_Device->WaitDeviceOnFinish();

    m_TrianglePipeline->Destroy();
    m_Swapchain->Destroy();

    m_Swapchain->Create();
    m_TrianglePipeline->Create();
}

void VulkanContext::Destroy()
{
    m_Device->WaitDeviceOnFinish();
    m_bIsDestroying = VK_TRUE;

    m_TriangleVertexBuffer->Destroy();
    m_TrianglePipeline->Destroy();

    m_MonkeyMesh->Destroy();

    m_GlobalRenderPass->Destroy();

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

    if (bEnableValidationLayers)
    {
        vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    vkDestroyInstance(m_Instance, nullptr);
    LOG_INFO("Vulkan context destroyed!");
}

}  // namespace Eclipse