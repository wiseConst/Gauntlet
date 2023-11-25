#pragma once

#include "Gauntlet/Renderer/CommandBuffer.h"
#include "VulkanUtility.h"

#include <volk/volk.h>

#include "Gauntlet/Core/Math.h"

namespace Gauntlet
{

class VulkanPipeline;

class VulkanCommandBuffer final : public CommandBuffer
{
  public:
    VulkanCommandBuffer(ECommandBufferType type, bool bIsSeparate = false);
    VulkanCommandBuffer()  = delete;
    ~VulkanCommandBuffer() = default;

    FORCEINLINE const auto& Get() const { return m_CommandBuffer; }
    FORCEINLINE auto& Get() { return m_CommandBuffer; }

    FORCEINLINE const auto& GetLevel() const { return m_Level; }
    FORCEINLINE auto& GetLevel() { return m_Level; }

    FORCEINLINE const std::vector<size_t>& GetTimestampResults() final override { return m_TimestampResults; }
    FORCEINLINE const std::vector<size_t>& GetStatisticsResults() final override { return m_StatsResults; }

    FORCEINLINE void Reset() const { VK_CHECK(vkResetCommandBuffer(m_CommandBuffer, 0), "Failed to reset command buffer!"); }
    void Destroy() final override;

    void BeginTimestamp(bool bStatisticsQuery = false) final override;
    void EndTimestamp(bool bStatisticsQuery = false) final override;

    // Call before render pass begin
    void BeginDebugLabel(const char* commandBufferLabelName = "NONAME", const glm::vec4& labelColor = glm::vec4(1.0f)) const;

    // Call after render pass end
    FORCEINLINE void EndDebugLabel() const
    {
        if (s_bEnableValidationLayers) vkCmdEndDebugUtilsLabelEXT(m_CommandBuffer);
    }

    void BeginRecording(const void* inheritanceInfo = VK_NULL_HANDLE) final override;
    FORCEINLINE void EndRecording() final override
    {
        VK_CHECK(vkEndCommandBuffer(m_CommandBuffer), "Failed to end recording command buffer");
    }

    void Submit() final override;

    FORCEINLINE void BindPushConstants(VkPipelineLayout pipelineLayout, const VkShaderStageFlags shaderStageFlags, const uint32_t offset,
                                       const uint32_t size, const void* values = VK_NULL_HANDLE) const
    {
        vkCmdPushConstants(m_CommandBuffer, pipelineLayout, shaderStageFlags, offset, size, values);
    }

    FORCEINLINE void BindDescriptorSets(VkPipelineLayout pipelineLayout, const uint32_t firstSet = 0, const uint32_t descriptorCount = 0,
                                        VkDescriptorSet* descriptorSets       = VK_NULL_HANDLE,
                                        VkPipelineBindPoint pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        const uint32_t dynamicOffsetCount = 0, uint32_t* dynamicOffsets = nullptr) const
    {
        vkCmdBindDescriptorSets(m_CommandBuffer, pipelineBindPoint, pipelineLayout, firstSet, descriptorCount, descriptorSets,
                                dynamicOffsetCount, dynamicOffsets);
    }

    void BindPipeline(Ref<VulkanPipeline>& pipeline, VkPipelineBindPoint pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS) const;
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

  private:
    VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
    VkCommandBufferLevel m_Level    = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    VkFence m_SubmitFence           = VK_NULL_HANDLE;
    bool m_bAllocatedSeparate       = false;
    ECommandBufferType m_Type       = ECommandBufferType::COMMAND_BUFFER_TYPE_GRAPHICS;

    VkQueryPool m_TimestampQueryPool = VK_NULL_HANDLE;
    std::vector<size_t> m_TimestampResults;
    VkQueryPool m_StatisticsQueryPool = VK_NULL_HANDLE;
    std::vector<size_t> m_StatsResults;

    uint32_t m_TimestampIndex = 0;

    friend class VulkanCommandPool;

    void CreateSyncResourcesAndQueries();
};

}  // namespace Gauntlet