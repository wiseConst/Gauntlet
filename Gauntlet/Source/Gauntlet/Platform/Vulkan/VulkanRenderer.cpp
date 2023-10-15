#include "GauntletPCH.h"
#include "VulkanRenderer.h"

#include "Gauntlet/Renderer/Camera/PerspectiveCamera.h"

#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"
#include "VulkanPipeline.h"
#include "VulkanFramebuffer.h"

#include "VulkanRenderer2D.h"
#include "VulkanUtility.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "VulkanTextureCube.h"
#include "VulkanBuffer.h"
#include "VulkanMaterial.h"

#include "Gauntlet/Renderer/Skybox.h"
#include "Gauntlet/Renderer/Mesh.h"

#include "Gauntlet/Core/Random.h"

#pragma warning(disable : 4834)

namespace Gauntlet
{

VulkanRenderer::VulkanRenderer() : m_Context((VulkanContext&)VulkanContext::Get())
{
    {
        const auto TextureDescriptorSetLayoutBinding =
            Utility::GetDescriptorSetLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        VkDescriptorSetLayoutCreateInfo ImageDescriptorSetLayoutCreateInfo =
            Utility::GetDescriptorSetLayoutCreateInfo(1, &TextureDescriptorSetLayoutBinding);
        VK_CHECK(vkCreateDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), &ImageDescriptorSetLayoutCreateInfo, nullptr,
                                             &s_Data.ImageDescriptorSetLayout),
                 "Failed to create texture(UI) descriptor set layout!");
    }
}

void VulkanRenderer::PostInit()
{
    // SetupSkybox();

    s_Data.GeometryDescriptorSetLayout =
        std::static_pointer_cast<VulkanShader>(s_RendererStorage->GeometryPipeline->GetSpecification().Shader)
            ->GetDescriptorSetLayouts()[0];
}

VulkanRenderer::~VulkanRenderer()
{
    m_Context.WaitDeviceOnFinish();

    vkDestroyDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), s_Data.ImageDescriptorSetLayout, nullptr);

    s_Data.CurrentCommandBuffer = nullptr;

    // DestroySkybox();

    for (auto& sampler : s_Data.Samplers)
        vkDestroySampler(m_Context.GetDevice()->GetLogicalDevice(), sampler.second, nullptr);
}

void VulkanRenderer::BeginSceneImpl(const Camera& camera)
{
    VulkanRenderer2D::GetStorageData().CameraProjectionMatrix = camera.GetViewProjectionMatrix();
    s_Data.SkyboxPushConstants.TransformMatrix =
        camera.GetProjectionMatrix() *
        glm::mat4(glm::mat3(camera.GetViewMatrix()));  // Removing 4th column of view martix which contains translation;
}

void VulkanRenderer::BeginImpl()
{
    s_Data.CurrentCommandBuffer = &m_Context.GetGraphicsCommandPool()->GetCommandBuffer(m_Context.GetSwapchain()->GetCurrentFrameIndex());
    GNT_ASSERT(s_Data.CurrentCommandBuffer, "Failed to retrieve command buffer!");
}

void VulkanRenderer::SetupSkybox()
{
    const auto SkyboxPath                = std::string("Resources/Textures/Skybox/Yokohama2/");
    const std::vector<std::string> Faces = {
        SkyboxPath + "right.jpg",   //
        SkyboxPath + "left.jpg",    //
        SkyboxPath + "bottom.jpg",  // Swapped because I flip the viewport's Y axis
        SkyboxPath + "top.jpg",     // Swapped
        SkyboxPath + "front.jpg",   //
        SkyboxPath + "back.jpg"     //
    };
    s_Data.DefaultSkybox = MakeRef<Skybox>(Faces);

    auto SkyboxShader          = std::static_pointer_cast<VulkanShader>(ShaderLibrary::Load("Resources/Cached/Shaders/Skybox"));
    s_Data.SkyboxDescriptorSet = SkyboxShader->GetDescriptorSets()[0];

    // Base skybox pipeline creation
    PipelineSpecification skyboxPipelineSpec = {};
    skyboxPipelineSpec.Name                  = "Skybox";
    skyboxPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
    skyboxPipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
    skyboxPipelineSpec.PrimitiveTopology     = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    skyboxPipelineSpec.CullMode              = ECullMode::CULL_MODE_BACK;
    skyboxPipelineSpec.bDepthTest            = VK_TRUE;

    skyboxPipelineSpec.TargetFramebuffer = s_RendererStorage->GeometryFramebuffer;
    skyboxPipelineSpec.Shader            = SkyboxShader;

    s_Data.SkyboxPipeline = MakeRef<VulkanPipeline>(skyboxPipelineSpec);
    SkyboxShader->Set("u_CubeMap", s_Data.DefaultSkybox->GetCubeMapTexture());
}

void VulkanRenderer::DrawSkybox()
{
    GNT_ASSERT(s_Data.CurrentCommandBuffer, "You can't render skybox before you've acquired valid command buffer!");
    s_Data.CurrentCommandBuffer->BeginDebugLabel("Skybox", glm::vec4(0.0f, 0.0f, 0.25f, 1.0f));

    static constexpr uint32_t s_MaxSkyboxSize = 500;
    s_Data.SkyboxPushConstants.TransformMatrix *= glm::scale(glm::mat4(1.0f), glm::vec3(s_MaxSkyboxSize));

    // const auto& CubeMesh = s_Data.DefaultSkybox->GetCubeMesh();
    //  DrawIndexedInternal(s_Data.SkyboxPipeline, CubeMesh->GetIndexBuffers()[0], CubeMesh->GetVertexBuffers()[0],
    //  &s_Data.SkyboxPushConstants,
    //                    &s_Data.SkyboxDescriptorSet.Handle, 1);

    s_Data.CurrentCommandBuffer->EndDebugLabel();
}

void VulkanRenderer::DestroySkybox()
{
    m_Context.GetDescriptorAllocator()->ReleaseDescriptorSets(&s_Data.SkyboxDescriptorSet, 1);
    s_Data.SkyboxPipeline->Destroy();
    s_Data.DefaultSkybox->Destroy();
}

void VulkanRenderer::EndSceneImpl()
{
    GNT_ASSERT(s_Data.CurrentCommandBuffer, "Failed to retrieve command buffer!");
}

void VulkanRenderer::FlushImpl()
{
    // Render skybox as last geometry to prevent depth testing as mush as possible
    /* s_Data.CurrentCommandBuffer->BeginDebugLabel("Skybox", glm::vec4(0.5f, 0.0f, 0.0f, 1.0f));
     s_Data.GeometryFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());

     DrawSkybox();

     s_Data.GeometryFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());
     s_Data.CurrentCommandBuffer->EndDebugLabel();*/
}

void VulkanRenderer::BeginRenderPassImpl(const Ref<Framebuffer>& framebuffer, const glm::vec4& debugLabelColor)
{
    GNT_ASSERT(s_Data.CurrentCommandBuffer);

    s_Data.CurrentCommandBuffer->BeginDebugLabel(framebuffer->GetSpecification().Name.data(), debugLabelColor);
    auto vulkanFramebuffer = std::static_pointer_cast<VulkanFramebuffer>(framebuffer);
    vulkanFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());
}

void VulkanRenderer::EndRenderPassImpl(const Ref<Framebuffer>& framebuffer)
{
    GNT_ASSERT(s_Data.CurrentCommandBuffer);

    auto vulkanFramebuffer = std::static_pointer_cast<VulkanFramebuffer>(framebuffer);
    vulkanFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());

    s_Data.CurrentCommandBuffer->EndDebugLabel();
}

void VulkanRenderer::RenderGeometryImpl(Ref<Pipeline>& pipeline, const GeometryData& geometry, bool bWithMaterial, void* pushConstants)
{
    if (bWithMaterial)
    {
        VkDescriptorSet ds = (VkDescriptorSet)geometry.Material->GetDescriptorSet();
        DrawIndexedInternal(pipeline, geometry.IndexBuffer, geometry.VertexBuffer, pushConstants, &ds, 1);
    }
    else
    {
        DrawIndexedInternal(pipeline, geometry.IndexBuffer, geometry.VertexBuffer, pushConstants);
    }
}

void VulkanRenderer::DrawIndexedInternal(Ref<Pipeline>& pipeline, const Ref<IndexBuffer>& indexBuffer,
                                         const Ref<VertexBuffer>& vertexBuffer, void* pushConstants, VkDescriptorSet* descriptorSets,
                                         const uint32_t descriptorCount)
{
    GNT_ASSERT(s_Data.CurrentCommandBuffer);

    auto vulkanPipeline = std::static_pointer_cast<VulkanPipeline>(pipeline);
    s_Data.CurrentCommandBuffer->SetPipelinePolygonMode(vulkanPipeline, GetSettings().ShowWireframes ? EPolygonMode::POLYGON_MODE_LINE
                                                                                                     : EPolygonMode::POLYGON_MODE_FILL);
    s_Data.CurrentCommandBuffer->BindPipeline(vulkanPipeline);

    if (pushConstants)
        s_Data.CurrentCommandBuffer->BindPushConstants(vulkanPipeline->GetLayout(), vulkanPipeline->GetPushConstantsShaderStageFlags(), 0,
                                                       vulkanPipeline->GetPushConstantsSize(), pushConstants);

    if (!descriptorSets)
    {
        auto vulkanShader = std::static_pointer_cast<VulkanShader>(pipeline->GetSpecification().Shader);
        std::vector<VkDescriptorSet> shaderDescriptorSets;
        for (auto& descriptorSet : vulkanShader->GetDescriptorSets())
            shaderDescriptorSets.push_back(descriptorSet.Handle);

        if (!shaderDescriptorSets.empty())
            s_Data.CurrentCommandBuffer->BindDescriptorSets(
                vulkanPipeline->GetLayout(), 0, static_cast<uint32_t>(shaderDescriptorSets.size()), shaderDescriptorSets.data());
    }
    else
    {
        s_Data.CurrentCommandBuffer->BindDescriptorSets(vulkanPipeline->GetLayout(), 0, descriptorCount, descriptorSets);
    }

    VkDeviceSize offset = 0;
    VkBuffer vb         = (VkBuffer)vertexBuffer->Get();
    s_Data.CurrentCommandBuffer->BindVertexBuffers(0, 1, &vb, &offset);

    VkBuffer ib = (VkBuffer)indexBuffer->Get();
    s_Data.CurrentCommandBuffer->BindIndexBuffer(ib);
    s_Data.CurrentCommandBuffer->DrawIndexed(static_cast<uint32_t>(indexBuffer->GetCount()));
    ++Renderer::GetStats().DrawCalls;
}

void VulkanRenderer::SubmitFullscreenQuadImpl(Ref<Pipeline>& pipeline, void* pushConstants)
{
    GNT_ASSERT(s_Data.CurrentCommandBuffer);

    auto vulkanPipeline = std::static_pointer_cast<VulkanPipeline>(pipeline);
    s_Data.CurrentCommandBuffer->BindPipeline(vulkanPipeline);

    if (pushConstants)
        s_Data.CurrentCommandBuffer->BindPushConstants(vulkanPipeline->GetLayout(), vulkanPipeline->GetPushConstantsShaderStageFlags(), 0,
                                                       vulkanPipeline->GetPushConstantsSize(), pushConstants);

    auto vulkanShader = std::static_pointer_cast<VulkanShader>(pipeline->GetSpecification().Shader);
    std::vector<VkDescriptorSet> descriptorSets;
    for (auto& descriptorSet : vulkanShader->GetDescriptorSets())
        descriptorSets.push_back(descriptorSet.Handle);

    if (!descriptorSets.empty())
        s_Data.CurrentCommandBuffer->BindDescriptorSets(vulkanPipeline->GetLayout(), 0, static_cast<uint32_t>(descriptorSets.size()),
                                                        descriptorSets.data());

    s_Data.CurrentCommandBuffer->Draw(3);
}
}  // namespace Gauntlet