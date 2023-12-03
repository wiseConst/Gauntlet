#include "GauntletPCH.h"
#include "VulkanRenderer.h"

#include "Gauntlet/Renderer/Camera/PerspectiveCamera.h"

#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanCommandBuffer.h"
#include "VulkanPipeline.h"
#include "VulkanFramebuffer.h"

#include "VulkanUtility.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "VulkanBuffer.h"
#include "VulkanMaterial.h"

#pragma warning(disable : 4834)

namespace Gauntlet
{

VulkanRenderer::VulkanRenderer() : m_Context((VulkanContext&)VulkanContext::Get())
{
    {
        // Stolen from ImGui
        const auto TextureDescriptorSetLayoutBinding =
            Utility::GetDescriptorSetLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        VkDescriptorSetLayoutCreateInfo ImageDescriptorSetLayoutCreateInfo =
            Utility::GetDescriptorSetLayoutCreateInfo(1, &TextureDescriptorSetLayoutBinding);
        VK_CHECK(vkCreateDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), &ImageDescriptorSetLayoutCreateInfo, nullptr,
                                             &s_Data.ImageDescriptorSetLayout),
                 "Failed to create texture(UI) descriptor set layout!");
    }
}

VulkanRenderer::~VulkanRenderer()
{
    m_Context.WaitDeviceOnFinish();

    vkDestroyDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), s_Data.ImageDescriptorSetLayout, nullptr);

    for (auto& sampler : s_Data.Samplers)
        vkDestroySampler(m_Context.GetDevice()->GetLogicalDevice(), sampler.second, nullptr);
}

void VulkanRenderer::BeginImpl()
{
    s_Data.CurrentPipelineToBind.reset();
}

void VulkanRenderer::SubmitParticleSystemImpl(const Ref<CommandBuffer>& commandBuffer, Ref<Pipeline>& pipeline, Ref<StorageBuffer>& ssbo,
                                              uint32_t particleCount, void* pushConstants)
{
    GNT_ASSERT(commandBuffer);
    auto cmdBuffer = std::static_pointer_cast<VulkanCommandBuffer>(commandBuffer);

    // Prevent same pipeline binding
    auto vulkanPipeline = std::static_pointer_cast<VulkanPipeline>(pipeline);
    if (!s_Data.CurrentPipelineToBind.lock() || s_Data.CurrentPipelineToBind.lock() != pipeline)
    {
        cmdBuffer->SetPipelinePolygonMode(vulkanPipeline,
                                          GetSettings().ShowWireframes ? EPolygonMode::POLYGON_MODE_LINE : EPolygonMode::POLYGON_MODE_FILL);
        cmdBuffer->BindPipeline(vulkanPipeline);

        s_Data.CurrentPipelineToBind = pipeline;
    }

    if (pushConstants)
        cmdBuffer->BindPushConstants(vulkanPipeline->GetLayout(), vulkanPipeline->GetPushConstantsShaderStageFlags(), 0,
                                     vulkanPipeline->GetPushConstantsSize(), pushConstants);

    auto vulkanShader = std::static_pointer_cast<VulkanShader>(vulkanPipeline->GetSpecification().Shader);
    std::vector<VkDescriptorSet> shaderDescriptorSets;
    for (auto& descriptorSet : vulkanShader->GetDescriptorSets())
        shaderDescriptorSets.push_back(descriptorSet.Handle);

    if (!shaderDescriptorSets.empty())
        cmdBuffer->BindDescriptorSets(vulkanPipeline, 0, static_cast<uint32_t>(shaderDescriptorSets.size()), shaderDescriptorSets.data());

    VkDeviceSize offset = 0;
    VkBuffer vb         = (VkBuffer)ssbo->Get();
    cmdBuffer->BindVertexBuffers(0, 1, &vb, &offset);

    cmdBuffer->Draw(particleCount);
    ++Renderer::GetStats().DrawCalls;
}

void VulkanRenderer::DispatchImpl(Ref<CommandBuffer>& commandBuffer, Ref<Pipeline>& pipeline, void* pushConstants,
                                  const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ)
{
    auto cmdBuffer = std::static_pointer_cast<VulkanCommandBuffer>(commandBuffer);

    auto vulkanPipeline = std::static_pointer_cast<VulkanPipeline>(pipeline);
    if (!s_Data.CurrentPipelineToBind.lock() || s_Data.CurrentPipelineToBind.lock() != pipeline)
    {
        cmdBuffer->BindPipeline(vulkanPipeline);

        s_Data.CurrentPipelineToBind = pipeline;
    }

    if (pushConstants)
        cmdBuffer->BindPushConstants(vulkanPipeline->GetLayout(), vulkanPipeline->GetPushConstantsShaderStageFlags(), 0,
                                     vulkanPipeline->GetPushConstantsSize(), pushConstants);

    auto vulkanShader = std::static_pointer_cast<VulkanShader>(pipeline->GetSpecification().Shader);
    std::vector<VkDescriptorSet> descriptorSets;
    for (auto& descriptorSet : vulkanShader->GetDescriptorSets())
        descriptorSets.push_back(descriptorSet.Handle);

    if (!descriptorSets.empty())
        cmdBuffer->BindDescriptorSets(vulkanPipeline, 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data());

    cmdBuffer->Dispatch(groupCountX, groupCountY, groupCountZ);
}

void VulkanRenderer::BeginRenderPassImpl(const Ref<CommandBuffer>& commandBuffer, const Ref<Framebuffer>& framebuffer,
                                         const glm::vec4& debugLabelColor)
{
    GNT_ASSERT(commandBuffer);
    auto cmdBuffer = std::static_pointer_cast<VulkanCommandBuffer>(commandBuffer);

    cmdBuffer->BeginDebugLabel(framebuffer->GetSpecification().Name.data(), debugLabelColor);
    auto vulkanFramebuffer = std::static_pointer_cast<VulkanFramebuffer>(framebuffer);
    vulkanFramebuffer->BeginRenderPass(cmdBuffer->Get());
}

void VulkanRenderer::EndRenderPassImpl(const Ref<CommandBuffer>& commandBuffer, const Ref<Framebuffer>& framebuffer)
{
    GNT_ASSERT(commandBuffer);
    auto cmdBuffer = std::static_pointer_cast<VulkanCommandBuffer>(commandBuffer);

    auto vulkanFramebuffer = std::static_pointer_cast<VulkanFramebuffer>(framebuffer);
    vulkanFramebuffer->EndRenderPass(cmdBuffer->Get());

    cmdBuffer->EndDebugLabel();
}

void VulkanRenderer::SubmitMeshImpl(Ref<Pipeline>& pipeline, Ref<VertexBuffer>& vertexBuffer, Ref<IndexBuffer>& indexBuffer,
                                    Ref<Material>& material, void* pushConstants)
{
    if (material)
    {
        VkDescriptorSet ds = (VkDescriptorSet)material->GetDescriptorSet();
        DrawIndexedInternal(pipeline, indexBuffer, vertexBuffer, pushConstants, &ds, 1);
    }
    else
        DrawIndexedInternal(pipeline, indexBuffer, vertexBuffer, pushConstants);
}

void VulkanRenderer::DrawIndexedInternal(Ref<Pipeline>& pipeline, const Ref<IndexBuffer>& indexBuffer,
                                         const Ref<VertexBuffer>& vertexBuffer, void* pushConstants, VkDescriptorSet* descriptorSets,
                                         const uint32_t descriptorCount)
{
    GNT_ASSERT(s_RendererStorage->RenderCommandBuffer[GraphicsContext::Get().GetCurrentFrameIndex()]);
    auto cmdBuffer = std::static_pointer_cast<VulkanCommandBuffer>(
        s_RendererStorage->RenderCommandBuffer[GraphicsContext::Get().GetCurrentFrameIndex()]);

    // Prevent same pipeline binding
    auto vulkanPipeline = std::static_pointer_cast<VulkanPipeline>(pipeline);
    if (!s_Data.CurrentPipelineToBind.lock() || s_Data.CurrentPipelineToBind.lock() != pipeline)
    {
        cmdBuffer->SetPipelinePolygonMode(vulkanPipeline,
                                          GetSettings().ShowWireframes ? EPolygonMode::POLYGON_MODE_LINE : EPolygonMode::POLYGON_MODE_FILL);
        cmdBuffer->BindPipeline(vulkanPipeline);

        s_Data.CurrentPipelineToBind = pipeline;
    }

    if (pushConstants)
        cmdBuffer->BindPushConstants(vulkanPipeline->GetLayout(), vulkanPipeline->GetPushConstantsShaderStageFlags(), 0,
                                     vulkanPipeline->GetPushConstantsSize(), pushConstants);

    if (!descriptorSets)
    {
        auto vulkanShader = std::static_pointer_cast<VulkanShader>(vulkanPipeline->GetSpecification().Shader);
        std::vector<VkDescriptorSet> shaderDescriptorSets;
        for (auto& descriptorSet : vulkanShader->GetDescriptorSets())
            shaderDescriptorSets.push_back(descriptorSet.Handle);

        if (!shaderDescriptorSets.empty())
            cmdBuffer->BindDescriptorSets(vulkanPipeline, 0, static_cast<uint32_t>(shaderDescriptorSets.size()),
                                          shaderDescriptorSets.data());
    }
    else
    {
        cmdBuffer->BindDescriptorSets(vulkanPipeline, 0, descriptorCount, descriptorSets);
    }

    VkDeviceSize offset = 0;
    VkBuffer vb         = (VkBuffer)vertexBuffer->Get();
    cmdBuffer->BindVertexBuffers(0, 1, &vb, &offset);

    VkBuffer ib = (VkBuffer)indexBuffer->Get();
    cmdBuffer->BindIndexBuffer(ib);
    cmdBuffer->DrawIndexed(static_cast<uint32_t>(indexBuffer->GetCount()));
    ++Renderer::GetStats().DrawCalls;
}

void VulkanRenderer::SubmitFullscreenQuadImpl(Ref<Pipeline>& pipeline, void* pushConstants)
{
    GNT_ASSERT(s_RendererStorage->RenderCommandBuffer[GraphicsContext::Get().GetCurrentFrameIndex()]);
    auto cmdBuffer = std::static_pointer_cast<VulkanCommandBuffer>(
        s_RendererStorage->RenderCommandBuffer[GraphicsContext::Get().GetCurrentFrameIndex()]);

    auto vulkanPipeline = std::static_pointer_cast<VulkanPipeline>(pipeline);
    if (!s_Data.CurrentPipelineToBind.lock() || s_Data.CurrentPipelineToBind.lock() != pipeline)
    {
        cmdBuffer->BindPipeline(vulkanPipeline);

        s_Data.CurrentPipelineToBind = pipeline;
    }

    if (pushConstants)
        cmdBuffer->BindPushConstants(vulkanPipeline->GetLayout(), vulkanPipeline->GetPushConstantsShaderStageFlags(), 0,
                                     vulkanPipeline->GetPushConstantsSize(), pushConstants);

    auto vulkanShader = std::static_pointer_cast<VulkanShader>(pipeline->GetSpecification().Shader);
    std::vector<VkDescriptorSet> descriptorSets;
    for (auto& descriptorSet : vulkanShader->GetDescriptorSets())
        descriptorSets.push_back(descriptorSet.Handle);

    if (!descriptorSets.empty())
        cmdBuffer->BindDescriptorSets(vulkanPipeline, 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data());

    cmdBuffer->Draw(3);
}

void VulkanRenderer::DrawQuadImpl(Ref<Pipeline>& pipeline, Ref<VertexBuffer>& vertexBuffer, Ref<IndexBuffer>& indexBuffer,
                                  const uint32_t indicesCount, void* pushConstants)
{
    GNT_ASSERT(s_RendererStorage->RenderCommandBuffer[GraphicsContext::Get().GetCurrentFrameIndex()]);
    auto cmdBuffer = std::static_pointer_cast<VulkanCommandBuffer>(
        s_RendererStorage->RenderCommandBuffer[GraphicsContext::Get().GetCurrentFrameIndex()]);

    auto vulkanPipeline = std::static_pointer_cast<VulkanPipeline>(pipeline);
    if (!s_Data.CurrentPipelineToBind.lock() || s_Data.CurrentPipelineToBind.lock() != pipeline)
    {

        cmdBuffer->SetPipelinePolygonMode(vulkanPipeline, Renderer::GetSettings().ShowWireframes ? EPolygonMode::POLYGON_MODE_LINE
                                                                                                 : EPolygonMode::POLYGON_MODE_FILL);
        cmdBuffer->BindPipeline(vulkanPipeline);

        s_Data.CurrentPipelineToBind = pipeline;
    }

    VkDeviceSize Offset = 0;
    VkBuffer vb         = (VkBuffer)vertexBuffer->Get();
    cmdBuffer->BindVertexBuffers(0, 1, &vb, &Offset);

    VkBuffer ib = (VkBuffer)indexBuffer->Get();
    cmdBuffer->BindIndexBuffer(ib);

    auto vulkanShader = std::static_pointer_cast<VulkanShader>(pipeline->GetSpecification().Shader);
    std::vector<VkDescriptorSet> descriptorSets;
    for (auto& descriptorSet : vulkanShader->GetDescriptorSets())
        descriptorSets.push_back(descriptorSet.Handle);

    if (!descriptorSets.empty())
        cmdBuffer->BindDescriptorSets(vulkanPipeline, 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data());

    if (pushConstants)
        cmdBuffer->BindPushConstants(vulkanPipeline->GetLayout(), vulkanPipeline->GetPushConstantsShaderStageFlags(), 0,
                                     vulkanPipeline->GetPushConstantsSize(), pushConstants);

    cmdBuffer->DrawIndexed(indicesCount);
}
}  // namespace Gauntlet