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

    s_PipelineStatNames = {"Input assembly vertex count        ", "Input assembly primitives count    ",
                           "Vertex shader invocations          "};

    auto& device                              = m_Context.GetDevice();
    VkQueryPoolCreateInfo queryPoolCreateInfo = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    // This query pool will store pipeline statistics
    queryPoolCreateInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
    // Pipeline counters to be returned for this pool
    queryPoolCreateInfo.pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |      // Input assembly vertices
                                             VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |    // Input assembly primitives
                                             VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |    // Vertex shader invocations
                                             VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |  // Fragment shader invocations
                                             VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |         // Clipping invocations
                                             VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |          // Clipped primitives
                                             VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;    // Compute shader invocations

    if (device->GetGPUFeatures().geometryShader)
    {
        queryPoolCreateInfo.pipelineStatistics |=
            VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT | VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT;

        s_PipelineStatNames.emplace_back("Geometry Shader Invocations        ");
        s_PipelineStatNames.emplace_back("Geometry Shader Primitives         ");
    }

    s_PipelineStatNames.emplace_back("Clipping stage primitives processed");
    s_PipelineStatNames.emplace_back("Clipping stage primitives output   ");
    s_PipelineStatNames.emplace_back("Fragment shader invocations        ");

    if (device->GetGPUFeatures().tessellationShader)
    {
        queryPoolCreateInfo.pipelineStatistics |= VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT |
                                                  VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT;

        s_PipelineStatNames.push_back("Tess. control shader patches       ");
        s_PipelineStatNames.push_back("Tess. eval. shader invocations     ");
    }

    s_PipelineStatNames.emplace_back("Compute shader invocations         ");

    queryPoolCreateInfo.queryCount = 1;

    VK_CHECK(vkCreateQueryPool(device->GetLogicalDevice(), &queryPoolCreateInfo, nullptr, &s_Data.QueryPool),
             "Failed to create query pool!");
    s_PipelineStats.resize(s_PipelineStatNames.size());
}

void VulkanRenderer::PostInit()
{
    // TODO: Get rid of it?
    s_Data.GeometryDescriptorSetLayout = /*
         std::static_pointer_cast<VulkanShader>(s_RendererStorage->GeometryPipeline->GetSpecification().Shader)
             ->GetDescriptorSetLayouts()[0];*/
        std::static_pointer_cast<VulkanShader>(s_RendererStorage->PBRPipeline->GetSpecification().Shader)->GetDescriptorSetLayouts()[0];
}

VulkanRenderer::~VulkanRenderer()
{
    m_Context.WaitDeviceOnFinish();

    vkDestroyQueryPool(m_Context.GetDevice()->GetLogicalDevice(), s_Data.QueryPool, nullptr);

    vkDestroyDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), s_Data.ImageDescriptorSetLayout, nullptr);

    s_Data.CurrentCommandBuffer = nullptr;

    for (auto& sampler : s_Data.Samplers)
        vkDestroySampler(m_Context.GetDevice()->GetLogicalDevice(), sampler.second, nullptr);
}

void VulkanRenderer::BeginImpl()
{
    s_Data.CurrentCommandBuffer = &m_Context.GetGraphicsCommandPool()->GetCommandBuffer(m_Context.GetSwapchain()->GetCurrentFrameIndex());
    GNT_ASSERT(s_Data.CurrentCommandBuffer, "Failed to retrieve command buffer!");

    s_Data.CurrentPipelineToBind.reset();
}

void VulkanRenderer::BeginQuery()
{
    vkCmdResetQueryPool(s_Data.CurrentCommandBuffer->Get(), s_Data.QueryPool, 0, 1);

    vkCmdBeginQuery(s_Data.CurrentCommandBuffer->Get(), s_Data.QueryPool, 0, 0);
}
void VulkanRenderer::EndQuery()
{
    vkCmdEndQuery(s_Data.CurrentCommandBuffer->Get(), s_Data.QueryPool, 0);
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
    GNT_ASSERT(s_Data.CurrentCommandBuffer);

    // Prevent same pipeline binding
    auto vulkanPipeline = std::static_pointer_cast<VulkanPipeline>(pipeline);
    if (!s_Data.CurrentPipelineToBind.lock() || s_Data.CurrentPipelineToBind.lock() != pipeline)
    {
        s_Data.CurrentCommandBuffer->SetPipelinePolygonMode(vulkanPipeline, GetSettings().ShowWireframes ? EPolygonMode::POLYGON_MODE_LINE
                                                                                                         : EPolygonMode::POLYGON_MODE_FILL);
        s_Data.CurrentCommandBuffer->BindPipeline(vulkanPipeline);

        s_Data.CurrentPipelineToBind = pipeline;
    }

    if (pushConstants)
        s_Data.CurrentCommandBuffer->BindPushConstants(vulkanPipeline->GetLayout(), vulkanPipeline->GetPushConstantsShaderStageFlags(), 0,
                                                       vulkanPipeline->GetPushConstantsSize(), pushConstants);

    if (!descriptorSets)
    {
        auto vulkanShader = std::static_pointer_cast<VulkanShader>(vulkanPipeline->GetSpecification().Shader);
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
    if (!s_Data.CurrentPipelineToBind.lock() || s_Data.CurrentPipelineToBind.lock() != pipeline)
    {
        s_Data.CurrentCommandBuffer->BindPipeline(vulkanPipeline);

        s_Data.CurrentPipelineToBind = pipeline;
    }

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

void VulkanRenderer::DrawQuadImpl(Ref<Pipeline>& pipeline, Ref<VertexBuffer>& vertexBuffer, Ref<IndexBuffer>& indexBuffer,
                                  const uint32_t indicesCount, void* pushConstants)
{
    GNT_ASSERT(s_Data.CurrentCommandBuffer);

    auto vulkanPipeline = std::static_pointer_cast<VulkanPipeline>(pipeline);
    if (!s_Data.CurrentPipelineToBind.lock() || s_Data.CurrentPipelineToBind.lock() != pipeline)
    {

        s_Data.CurrentCommandBuffer->SetPipelinePolygonMode(
            vulkanPipeline, Renderer::GetSettings().ShowWireframes ? EPolygonMode::POLYGON_MODE_LINE : EPolygonMode::POLYGON_MODE_FILL);
        s_Data.CurrentCommandBuffer->BindPipeline(vulkanPipeline);

        s_Data.CurrentPipelineToBind = pipeline;
    }

    VkDeviceSize Offset = 0;
    VkBuffer vb         = (VkBuffer)vertexBuffer->Get();
    s_Data.CurrentCommandBuffer->BindVertexBuffers(0, 1, &vb, &Offset);

    VkBuffer ib = (VkBuffer)indexBuffer->Get();
    s_Data.CurrentCommandBuffer->BindIndexBuffer(ib);

    auto vulkanShader = std::static_pointer_cast<VulkanShader>(pipeline->GetSpecification().Shader);
    std::vector<VkDescriptorSet> descriptorSets;
    for (auto& descriptorSet : vulkanShader->GetDescriptorSets())
        descriptorSets.push_back(descriptorSet.Handle);

    if (!descriptorSets.empty())
        s_Data.CurrentCommandBuffer->BindDescriptorSets(vulkanPipeline->GetLayout(), 0, static_cast<uint32_t>(descriptorSets.size()),
                                                        descriptorSets.data());

    if (pushConstants)
        s_Data.CurrentCommandBuffer->BindPushConstants(vulkanPipeline->GetLayout(), vulkanPipeline->GetPushConstantsShaderStageFlags(), 0,
                                                       vulkanPipeline->GetPushConstantsSize(), pushConstants);

    s_Data.CurrentCommandBuffer->DrawIndexed(indicesCount);
}
}  // namespace Gauntlet