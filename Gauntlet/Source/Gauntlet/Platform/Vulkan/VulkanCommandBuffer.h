#pragma once

#include "Gauntlet/Renderer/CommandBuffer.h"
#include "VulkanUtility.h"

#include <volk/volk.h>

namespace Gauntlet
{

class VulkanPipeline;

class VulkanCommandBuffer final : public CommandBuffer
{
  public:
    VulkanCommandBuffer(ECommandBufferType type, ECommandBufferLevel level = ECommandBufferLevel::COMMAND_BUFFER_LEVEL_PRIMARY);
    VulkanCommandBuffer() = delete;
    ~VulkanCommandBuffer() { Destroy(); }

    FORCEINLINE const auto& Get() const { return m_CommandBuffer; }
    FORCEINLINE auto& Get() { return m_CommandBuffer; }

    FORCEINLINE ECommandBufferLevel GetLevel() const final override { return m_Level; }

    FORCEINLINE const std::vector<size_t>& GetTimestampResults() const final override { return m_TimestampResults; }
    FORCEINLINE const std::vector<size_t>& GetPipelineStatisticsResults() const final override { return m_PipelineStatsResults; }
    const std::vector<std::string> GetPipelineStatisticsStrings() const final override;

    FORCEINLINE void Reset() final override { VK_CHECK(vkResetCommandBuffer(m_CommandBuffer, 0), "Failed to reset command buffer!"); }

    void BeginTimestamp(bool bStatisticsQuery = false) final override;
    void EndTimestamp(bool bStatisticsQuery = false) final override;

    void BeginDebugLabel(const char* commandBufferLabelName, const glm::vec4& labelColor) const final override;
    FORCEINLINE void EndDebugLabel() const final override
    {
        if (s_bEnableValidationLayers) vkCmdEndDebugUtilsLabelEXT(m_CommandBuffer);
    }

    void BeginRecording(bool bOneTimeSubmit = false, const void* inheritanceInfo = VK_NULL_HANDLE) final override;
    FORCEINLINE void EndRecording() final override
    {
        VK_CHECK(vkEndCommandBuffer(m_CommandBuffer), "Failed to end recording command buffer");
    }

    void Submit(bool bWaitAfterSubmit = true) final override;

    FORCEINLINE void InsertBarrier(const VkPipelineStageFlags srcStageMask, const VkPipelineStageFlags dstStageMask,
                                   const VkDependencyFlags dependencyFlags, const uint32_t memoryBarrierCount,
                                   const VkMemoryBarrier* pMemoryBarriers, const uint32_t bufferMemoryBarrierCount,
                                   const VkBufferMemoryBarrier* pBufferMemoryBarriers, const uint32_t imageMemoryBarrierCount,
                                   const VkImageMemoryBarrier* pImageMemoryBarriers)
    {
        vkCmdPipelineBarrier(m_CommandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers,
                             bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
    }

    FORCEINLINE void BindPushConstants(VkPipelineLayout pipelineLayout, const VkShaderStageFlags shaderStageFlags, const uint32_t offset,
                                       const uint32_t size, const void* values = VK_NULL_HANDLE) const
    {
        vkCmdPushConstants(m_CommandBuffer, pipelineLayout, shaderStageFlags, offset, size, values);
    }

    void BindDescriptorSets(Ref<VulkanPipeline>& pipeline, const uint32_t firstSet = 0, const uint32_t descriptorSetCount = 0,
                            VkDescriptorSet* descriptorSets = VK_NULL_HANDLE, const uint32_t dynamicOffsetCount = 0,
                            uint32_t* dynamicOffsets = nullptr) const;

    void BindPipeline(Ref<VulkanPipeline>& pipeline) const;
    void SetPipelinePolygonMode(Ref<VulkanPipeline>& pipeline,
                                const EPolygonMode polygonMode) const;  // Invoke before binding actual pipeline

    FORCEINLINE void DrawIndexed(const uint32_t indexCount, const uint32_t instanceCount = 1, const uint32_t firstIndex = 0,
                                 const int32_t vertexOffset = 0, const uint32_t firstInstance = 0) const
    {
        vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    FORCEINLINE void Draw(const uint32_t vertexCount, const uint32_t instanceCount = 1, const uint32_t firstVertex = 0,
                          const uint32_t firstInstance = 0) const
    {
        vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    FORCEINLINE void DrawIndexedIndirect(const VkBuffer& buffer, const VkDeviceSize offset, const uint32_t drawCount, const uint32_t stride)
    {
        vkCmdDrawIndexedIndirect(m_CommandBuffer, buffer, offset, drawCount, stride);
    }

    FORCEINLINE void BindVertexBuffers(const uint32_t firstBinding = 0, const uint32_t bindingCount = 1, VkBuffer* buffers = VK_NULL_HANDLE,
                                       VkDeviceSize* offsets = VK_NULL_HANDLE) const
    {
        GNT_ASSERT(buffers, "Invalid vertex buffer(s)!");
        vkCmdBindVertexBuffers(m_CommandBuffer, firstBinding, bindingCount, buffers, offsets);
    }

    FORCEINLINE void Dispatch(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ)
    {
        vkCmdDispatch(m_CommandBuffer, groupCountX, groupCountY, groupCountZ);
    }

    FORCEINLINE
    void BindIndexBuffer(const VkBuffer& buffer, const VkDeviceSize offset = 0, VkIndexType indexType = VK_INDEX_TYPE_UINT32) const
    {
        vkCmdBindIndexBuffer(m_CommandBuffer, buffer, offset, indexType);
    }

    FORCEINLINE void TraceRays(const VkStridedDeviceAddressRegionKHR* raygenShaderBindingTable,
                               const VkStridedDeviceAddressRegionKHR* missShaderBindingTable,
                               const VkStridedDeviceAddressRegionKHR* hitShaderBindingTable,
                               const VkStridedDeviceAddressRegionKHR* callableShaderBindingTable, uint32_t width, uint32_t height,
                               uint32_t depth = 1) const
    {
        vkCmdTraceRaysKHR(m_CommandBuffer, raygenShaderBindingTable, missShaderBindingTable, hitShaderBindingTable,
                          callableShaderBindingTable, width, height, depth);
    }

    FORCEINLINE void BuildAccelerationStructure(uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR* infos,
                                                const VkAccelerationStructureBuildRangeInfoKHR* const* buildRangeInfos)
    {
        vkCmdBuildAccelerationStructuresKHR(m_CommandBuffer, infoCount, infos, buildRangeInfos);
    }

    FORCEINLINE void CopyBuffer(const VkBuffer& srcBuffer, VkBuffer& dstBuffer, const uint32_t regionCount, const VkBufferCopy* pRegions)
    {
        vkCmdCopyBuffer(m_CommandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
    }

    FORCEINLINE void CopyBufferToImage(const VkBuffer& srcBuffer, VkImage& dstImage, const VkImageLayout dstImageLayout,
                                       const uint32_t regionCount, const VkBufferImageCopy* pRegions)
    {
        vkCmdCopyBufferToImage(m_CommandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
    }

    FORCEINLINE void BlitImage(const VkImage& srcImage, const VkImageLayout srcImageLayout, VkImage& dstImage,
                               const VkImageLayout dstImageLayout, const uint32_t regionCount, const VkImageBlit* pRegions,
                               const VkFilter filter)
    {
        vkCmdBlitImage(m_CommandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
    }

  private:
    VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
    ECommandBufferLevel m_Level     = ECommandBufferLevel::COMMAND_BUFFER_LEVEL_PRIMARY;
    ECommandBufferType m_Type       = ECommandBufferType::COMMAND_BUFFER_TYPE_GRAPHICS;
    VkFence m_SubmitFence           = VK_NULL_HANDLE;

    VkQueryPool m_TimestampQueryPool = VK_NULL_HANDLE;
    std::vector<size_t> m_TimestampResults;

    VkQueryPool m_PipelineStatisticsQueryPool = VK_NULL_HANDLE;
    std::vector<size_t> m_PipelineStatsResults;

    uint32_t m_TimestampIndex = 0;
    void CreateSyncResourcesAndQueries();
    void Destroy() final override;
};

}  // namespace Gauntlet