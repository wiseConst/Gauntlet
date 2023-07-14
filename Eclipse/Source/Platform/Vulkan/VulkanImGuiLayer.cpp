#include "EclipsePCH.h"
#include "VulkanImGuiLayer.h"

#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/backends/imgui_impl_glfw.h>

#include "Eclipse/Core/Application.h"
#include "Eclipse/Core/Window.h"

#include "VulkanUtility.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanCommandBuffer.h"

namespace Eclipse
{
void VulkanImGuiLayer::OnAttach()
{
    auto& Context                                       = (VulkanContext&)VulkanContext::Get();
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
    VK_CHECK(vkCreateDescriptorPool(Context.GetDevice()->GetLogicalDevice(), &DescriptorPoolCreateInfo, nullptr, &m_ImGuiDescriptorPool),
             "Failed to create imgui descriptor pool!");

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::SetCurrentContext(ImGui::CreateContext());
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Enable Docking
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // Enable Multi-Viewport / Platform Windows

    // io.ConfigViewportsNoAutoMerge = true;
    // io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    // ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();
    SetStyle();

    const auto& App = Application::Get();
    ImGui_ImplVulkan_LoadFunctions([](const char* FunctionName, void* VulkanInstance)
                                   { return vkGetInstanceProcAddr(*(reinterpret_cast<VkInstance*>(VulkanInstance)), FunctionName); },
                                   &Context.GetInstance());

    // ImGui RenderPass
    {
        RenderPassSpecification RenderPassSpec  = {};
        VkAttachmentDescription ColorAttachment = {};
        ColorAttachment.format                  = Context.GetSwapchain()->GetImageFormat();
        ColorAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
        ColorAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_LOAD;
        ColorAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
        ColorAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        ColorAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        ColorAttachment.initialLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        ColorAttachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        RenderPassSpec.Attachments = {ColorAttachment};

        VkAttachmentReference ColorAttachmentRef = {};
        ColorAttachmentRef.attachment            = 0;
        ColorAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        RenderPassSpec.AttachmentRefs = {ColorAttachmentRef};

        VkSubpassDescription Subpass = {};
        Subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        Subpass.colorAttachmentCount = 1;
        Subpass.pColorAttachments    = &ColorAttachmentRef;
        RenderPassSpec.Subpasses     = {Subpass};

        VkSubpassDependency Dependency = {};
        Dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
        Dependency.dstSubpass          = 0;
        Dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        Dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        Dependency.srcAccessMask       = 0;
        Dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        RenderPassSpec.Dependencies = {Dependency};

        m_ImGuiRenderPass.reset(new VulkanRenderPass(RenderPassSpec));
        Context.AddResizeCallback([this] { m_ImGuiRenderPass->Invalidate(); });  // Auto render pass resize
    }

    // ImGui CommandBuffers
    {
        CommandPoolSpecification CommandPoolSpec = {};
        CommandPoolSpec.CommandPoolUsage         = ECommandPoolUsage::COMMAND_POOL_DEFAULT_USAGE;
        CommandPoolSpec.QueueFamilyIndex         = Context.GetDevice()->GetQueueFamilyIndices().GraphicsFamily;

        m_ImGuiCommandPool.reset(new VulkanCommandPool(CommandPoolSpec));
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)App.GetWindow()->GetNativeWindow(), true);
    ImGui_ImplVulkan_InitInfo InitInfo = {};
    InitInfo.Instance                  = Context.GetInstance();
    InitInfo.PhysicalDevice            = Context.GetDevice()->GetPhysicalDevice();
    InitInfo.Device                    = Context.GetDevice()->GetLogicalDevice();
    InitInfo.QueueFamily               = Context.GetDevice()->GetQueueFamilyIndices().GraphicsFamily;
    InitInfo.Queue                     = Context.GetDevice()->GetGraphicsQueue();
    InitInfo.PipelineCache             = VK_NULL_HANDLE;
    InitInfo.DescriptorPool            = m_ImGuiDescriptorPool;
    InitInfo.MinImageCount             = Context.GetSwapchain()->GetImageCount();
    InitInfo.ImageCount                = Context.GetSwapchain()->GetImageCount();
    InitInfo.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
    InitInfo.Allocator                 = VK_NULL_HANDLE;
    ImGui_ImplVulkan_Init(&InitInfo, m_ImGuiRenderPass->Get());

    // Load default font
    {
        m_DefaultFont  = io.Fonts->AddFontFromFileTTF("Resources/Fonts/Rubik-Regular.ttf", 18.0f);
        io.FontDefault = m_DefaultFont;

        auto CommandBuffer = Utility::BeginSingleTimeCommands(m_ImGuiCommandPool->Get(), Context.GetDevice()->GetLogicalDevice());
        ImGui_ImplVulkan_CreateFontsTexture(CommandBuffer);

        Utility::EndSingleTimeCommands(CommandBuffer, m_ImGuiCommandPool->Get(), Context.GetDevice()->GetGraphicsQueue(),
                                       Context.GetDevice()->GetLogicalDevice());
        Context.GetDevice()->WaitDeviceOnFinish();
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

#if ELS_DEBUG
    LOG_INFO("ImGui created!");
#endif
}

void VulkanImGuiLayer::OnDetach()
{
    const auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    Context.GetDevice()->WaitDeviceOnFinish();
    vkDestroyDescriptorPool(Context.GetDevice()->GetLogicalDevice(), m_ImGuiDescriptorPool, nullptr);

    m_ImGuiRenderPass->Destroy();
    m_ImGuiCommandPool->Destroy();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void VulkanImGuiLayer::BeginRender()
{
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    const auto& CommandBuffer = m_ImGuiCommandPool->GetCommandBuffer(Context.GetSwapchain()->GetCurrentFrameIndex());
    CommandBuffer.BeginRecording();

    m_ImGuiRenderPass->Begin(CommandBuffer.Get(), {{0.1f, 0.1f, 0.1f, 1.0f}});

    /*static bool ShowDemoWindow = true;
    if (ShowDemoWindow) ImGui::ShowDemoWindow(&ShowDemoWindow);*/
}

void VulkanImGuiLayer::EndRender()
{
    const auto& Context       = (VulkanContext&)VulkanContext::Get();
    const auto& CommandBuffer = m_ImGuiCommandPool->GetCommandBuffer(Context.GetSwapchain()->GetCurrentFrameIndex());

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), CommandBuffer.Get());

    // Update and Render additional Platform Windows
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* BackupCurrentContext = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(BackupCurrentContext);
    }

    m_ImGuiRenderPass->End(CommandBuffer.Get());
    CommandBuffer.EndRecording();

    VkSubmitInfo SubmitInfo       = {};
    SubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers    = &CommandBuffer.Get();

    VK_CHECK(vkQueueSubmit(Context.GetDevice()->GetGraphicsQueue(), 1, &SubmitInfo, VK_NULL_HANDLE),
             "Failed to submit imgui render commands!");
}

void VulkanImGuiLayer::SetStyle()
{
    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io       = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding              = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    style.Colors[ImGuiCol_Text]         = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);

    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.258f, 0.258f, 0.258f, 1.000f);
    style.Colors[ImGuiCol_ChildBg]  = ImVec4(0.280f, 0.280f, 0.280f, 0.000f);
    style.Colors[ImGuiCol_PopupBg]  = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);

    style.Colors[ImGuiCol_Border]       = ImVec4(0.137f, 0.137f, 0.137f, 1.000f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);

    style.Colors[ImGuiCol_FrameBg]        = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
    style.Colors[ImGuiCol_FrameBgActive]  = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);

    style.Colors[ImGuiCol_TitleBg]          = ImVec4(1.000f, 0.000f, 0.000f, 1.000f);
    style.Colors[ImGuiCol_TitleBgActive]    = ImVec4(1.000f, 0.000f, 0.000f, 1.000f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);

    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.000f, 0.000f, 0.000f, 1.000f);

    style.Colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
    style.Colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.4f, 0.67f, 1.000f, 1.000f);

    style.Colors[ImGuiCol_CheckMark] = ImVec4(1.000f, 0.000f, 0.000f, 1.000f);

    style.Colors[ImGuiCol_SliderGrab]       = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.4f, 0.67f, 1.000f, 1.000f);

    style.Colors[ImGuiCol_Button]        = ImVec4(0.258f, 0.258f, 0.258f, 1.000f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
    style.Colors[ImGuiCol_ButtonActive]  = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);

    style.Colors[ImGuiCol_Header]        = ImVec4(1.000f, 0.000f, 0.000f, 1.000f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
    style.Colors[ImGuiCol_HeaderActive]  = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);

    style.Colors[ImGuiCol_Separator]        = style.Colors[ImGuiCol_Border];
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
    style.Colors[ImGuiCol_SeparatorActive]  = ImVec4(0.4f, 0.67f, 1.000f, 1.000f);

    style.Colors[ImGuiCol_ResizeGrip]        = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
    style.Colors[ImGuiCol_ResizeGripActive]  = ImVec4(0.4f, 0.67f, 1.000f, 1.000f);

    style.Colors[ImGuiCol_Tab] = ImVec4(0.137f, 0.137f, 0.137f, 1.000f);

    style.Colors[ImGuiCol_TabHovered]         = ImVec4(0.352f, 0.352f, 0.352f, 1.000f);
    style.Colors[ImGuiCol_TabActive]          = ImVec4(0.258f, 0.258f, 0.258f, 1.000f);
    style.Colors[ImGuiCol_TabUnfocused]       = ImVec4(0.137f, 0.137f, 0.137f, 1.000f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.258f, 0.258f, 0.258f, 1.000f);

    style.Colors[ImGuiCol_DockingPreview] = ImVec4(0.4f, 0.67f, 1.000f, 0.781f);
    style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.137f, 0.137f, 0.137f, 1.000f);

    style.Colors[ImGuiCol_TableRowBg]    = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);

    style.Colors[ImGuiCol_PlotLines]            = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
    style.Colors[ImGuiCol_PlotLinesHovered]     = ImVec4(0.4f, 0.67f, 1.000f, 1.000f);
    style.Colors[ImGuiCol_PlotHistogram]        = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);

    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);

    style.Colors[ImGuiCol_NavHighlight]          = ImVec4(0.4f, 0.67f, 1.000f, 1.000f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.4f, 0.67f, 1.000f, 1.000f);
    style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
    style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);

    style.ChildRounding     = 0;
    style.FrameRounding     = 9;
    style.GrabMinSize       = 7.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.ScrollbarSize     = 12.0f;
    style.TabBorderSize     = 0.0f;
    style.TabRounding       = 6.0f;
    style.WindowRounding    = 6.0f;
    style.WindowBorderSize  = 5.f;
}

}  // namespace Eclipse