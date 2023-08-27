#include "GauntletPCH.h"
#include "VulkanImGuiLayer.h"

#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/backends/imgui_impl_glfw.h>

#include "Gauntlet/Core/Application.h"
#include "Gauntlet/Core/Window.h"

#include "VulkanUtility.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"
#include "VulkanRenderer.h"

namespace Gauntlet
{
VulkanImGuiLayer::VulkanImGuiLayer()
    : ImGuiLayer("ImGuiLayer"), m_ImGuiCommandPool(nullptr), m_Context((VulkanContext&)VulkanContext::Get())
{
}

void VulkanImGuiLayer::OnAttach()
{
    const auto& Device = m_Context.GetDevice();

    VkDescriptorPoolSize PoolSizes[]                    = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};
    VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo = {};
    DescriptorPoolCreateInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    DescriptorPoolCreateInfo.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    DescriptorPoolCreateInfo.maxSets                    = 1000 * IM_ARRAYSIZE(PoolSizes);
    DescriptorPoolCreateInfo.poolSizeCount              = (uint32_t)IM_ARRAYSIZE(PoolSizes);
    DescriptorPoolCreateInfo.pPoolSizes                 = PoolSizes;
    VK_CHECK(vkCreateDescriptorPool(Device->GetLogicalDevice(), &DescriptorPoolCreateInfo, nullptr, &m_ImGuiDescriptorPool),
             "Failed to create imgui descriptor pool!");

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::SetCurrentContext(ImGui::CreateContext());
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_ViewportsEnable | ImGuiConfigFlags_DockingEnable |
                      ImGuiConfigFlags_DpiEnableScaleFonts | ImGuiConfigFlags_DpiEnableScaleViewports;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

    SetCustomUIStyle();

    const auto& App = Application::Get();
    ImGui_ImplVulkan_LoadFunctions([](const char* FunctionName, void* VulkanInstance)
                                   { return vkGetInstanceProcAddr(*(reinterpret_cast<VkInstance*>(VulkanInstance)), FunctionName); },
                                   &m_Context.GetInstance());

    const auto& Swapchain = m_Context.GetSwapchain();

    // ImGui CommandBuffers
    {
        CommandPoolSpecification CommandPoolSpec = {};
        CommandPoolSpec.CommandPoolUsage         = ECommandPoolUsage::COMMAND_POOL_DEFAULT_USAGE;
        CommandPoolSpec.QueueFamilyIndex         = Device->GetQueueFamilyIndices().GraphicsFamily;

        m_ImGuiCommandPool.reset(new VulkanCommandPool(CommandPoolSpec));
    }

    const uint32_t ImageCount = Swapchain->GetImageCount();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)App.GetWindow()->GetNativeWindow(), true);
    ImGui_ImplVulkan_InitInfo InitInfo = {};
    InitInfo.Instance                  = m_Context.GetInstance();
    InitInfo.PhysicalDevice            = Device->GetPhysicalDevice();
    InitInfo.Device                    = Device->GetLogicalDevice();
    InitInfo.QueueFamily               = Device->GetQueueFamilyIndices().GraphicsFamily;
    InitInfo.Queue                     = Device->GetGraphicsQueue();
    InitInfo.PipelineCache             = VK_NULL_HANDLE;
    InitInfo.DescriptorPool            = m_ImGuiDescriptorPool;
    InitInfo.MinImageCount             = ImageCount;
    InitInfo.ImageCount                = ImageCount;
    InitInfo.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
    InitInfo.Allocator                 = VK_NULL_HANDLE;
    ImGui_ImplVulkan_Init(&InitInfo, m_Context.GetSwapchain()->GetRenderPass());

    // Load default font
    {
        auto FontPath  = std::string("../Resources/Fonts/Tektur/Tektur-Regular.ttf");
        io.FontDefault = io.Fonts->AddFontFromFileTTF(FontPath.data(), 18.0f);
        FontPath       = std::string("../Resources/Fonts/Tektur/Tektur-Bold.ttf");
        io.Fonts->AddFontFromFileTTF(FontPath.data(), 18.0f);

        auto CommandBuffer = Utility::BeginSingleTimeCommands(m_ImGuiCommandPool->Get(), Device->GetLogicalDevice());
        ImGui_ImplVulkan_CreateFontsTexture(CommandBuffer);

        Utility::EndSingleTimeCommands(CommandBuffer, m_ImGuiCommandPool->Get(), Device->GetGraphicsQueue(), Device->GetLogicalDevice());
        m_Context.GetDevice()->WaitDeviceOnFinish();
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    CreateSyncObjects();
}

void VulkanImGuiLayer::OnDetach()
{
    m_Context.GetDevice()->WaitDeviceOnFinish();
    vkDestroyDescriptorPool(m_Context.GetDevice()->GetLogicalDevice(), m_ImGuiDescriptorPool, nullptr);

    m_ImGuiCommandPool->Destroy();
    m_CurrentCommandBuffer = nullptr;

    for (auto& InFlightFence : m_InFlightFences)
        vkDestroyFence(m_Context.GetDevice()->GetLogicalDevice(), InFlightFence, nullptr);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void VulkanImGuiLayer::BeginRender()
{
    VK_CHECK(vkWaitForFences(m_Context.GetDevice()->GetLogicalDevice(), 1,
                             &m_InFlightFences[m_Context.GetSwapchain()->GetCurrentFrameIndex()], VK_TRUE, UINT64_MAX),
             "Failed to wait for UI Render fence!");

    VK_CHECK(
        vkResetFences(m_Context.GetDevice()->GetLogicalDevice(), 1, &m_InFlightFences[m_Context.GetSwapchain()->GetCurrentFrameIndex()]),
        "Failed to reset UI render fence!");

    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    m_CurrentCommandBuffer = &m_ImGuiCommandPool->GetCommandBuffer(m_Context.GetSwapchain()->GetCurrentFrameIndex());
    GNT_ASSERT(m_CurrentCommandBuffer, "Failed to retrieve imgui command buffer!");

    m_CurrentCommandBuffer->BeginRecording();
    m_CurrentCommandBuffer->BeginDebugLabel("Swapchain + UI Pass", glm::vec4(0.0f, 0.0f, 0.8f, 1.0f));

    m_Context.GetSwapchain()->BeginRenderPass(m_CurrentCommandBuffer->Get());
}

void VulkanImGuiLayer::EndRender()
{
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_CurrentCommandBuffer->Get());

    // Update and Render additional Platform Windows
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* BackupCurrentContext = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(BackupCurrentContext);
    }

    m_Context.GetSwapchain()->EndRenderPass(m_CurrentCommandBuffer->Get());
    m_CurrentCommandBuffer->EndDebugLabel();
    m_CurrentCommandBuffer->EndRecording();

    VkSubmitInfo SubmitInfo       = {};
    SubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers    = &m_CurrentCommandBuffer->Get();

    VK_CHECK(vkQueueSubmit(m_Context.GetDevice()->GetGraphicsQueue(), 1, &SubmitInfo,
                           m_InFlightFences[m_Context.GetSwapchain()->GetCurrentFrameIndex()]),
             "Failed to submit imgui render commands!");
}

void VulkanImGuiLayer::OnEvent(Event& event)
{
    if (m_bBlockEvents)
    {
        ImGuiIO& io = ImGui::GetIO();
        if (event.GetType() <= EEventType::MouseButtonRepeatedEvent && io.WantCaptureMouse) event.SetHandled(true);
        if (event.GetType() <= EEventType::KeyButtonRepeatedEvent && io.WantCaptureKeyboard) event.SetHandled(true);
    }
}

void VulkanImGuiLayer::CreateSyncObjects()
{
    VkFenceCreateInfo FenceCreateInfo = {};
    FenceCreateInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    FenceCreateInfo.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

    m_InFlightFences.resize(FRAMES_IN_FLIGHT);
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        vkCreateFence(m_Context.GetDevice()->GetLogicalDevice(), &FenceCreateInfo, nullptr, &m_InFlightFences[i]);
    }
}

void VulkanImGuiLayer::SetCustomUIStyle()
{
    // Setup Dear ImGui style
    // ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    auto& colors              = ImGui::GetStyle().Colors;
    colors[ImGuiCol_WindowBg] = ImVec4{0.1f, 0.105f, 0.11f, 1.0f};

    // Headers
    colors[ImGuiCol_Header]        = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
    colors[ImGuiCol_HeaderHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
    colors[ImGuiCol_HeaderActive]  = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

    // Buttons
    colors[ImGuiCol_Button]        = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
    colors[ImGuiCol_ButtonHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
    colors[ImGuiCol_ButtonActive]  = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

    // Frame BG
    colors[ImGuiCol_FrameBg]        = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
    colors[ImGuiCol_FrameBgHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
    colors[ImGuiCol_FrameBgActive]  = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

    // Tabs
    colors[ImGuiCol_Tab]                = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TabHovered]         = ImVec4{0.38f, 0.3805f, 0.381f, 1.0f};
    colors[ImGuiCol_TabActive]          = ImVec4{0.28f, 0.2805f, 0.281f, 1.0f};
    colors[ImGuiCol_TabUnfocused]       = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};

    // Title
    colors[ImGuiCol_TitleBg]          = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TitleBgActive]    = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
}

}  // namespace Gauntlet