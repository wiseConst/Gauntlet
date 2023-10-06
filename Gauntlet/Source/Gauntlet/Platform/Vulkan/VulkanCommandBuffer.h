#pragma once

#include "Gauntlet/Core/Core.h"
#include "VulkanUtility.h"

#include <volk/volk.h>

#include <glm/glm.hpp>

namespace Gauntlet
{

class VulkanPipeline;

class VulkanCommandBuffer final /*: private Uncopyable, private Unmovable*/
{
  public:
    VulkanCommandBuffer()  = default;
    ~VulkanCommandBuffer() = default;

    FORCEINLINE const auto& Get() const { return m_CommandBuffer; }
    FORCEINLINE auto& Get() { return m_CommandBuffer; }

    FORCEINLINE const auto& GetLevel() const { return m_Level; }

    // Unused
    FORCEINLINE void Reset() const { VK_CHECK(vkResetCommandBuffer(m_CommandBuffer, 0), "Failed to reset command buffer!"); }

    // Call before render pass begin
    void BeginDebugLabel(const char* commandBufferLabelName = "NONAME", const glm::vec4& labelColor = glm::vec4(1.0f)) const;

    // Call after render pass end
    FORCEINLINE void EndDebugLabel() const
    {
        if (s_bEnableValidationLayers) vkCmdEndDebugUtilsLabelEXT(m_CommandBuffer);
    }

    void BeginRecording(const VkCommandBufferUsageFlags commandBufferUsageFlags = 0,
                        const VkCommandBufferInheritanceInfo* inheritanceInfo   = VK_NULL_HANDLE) const;
    FORCEINLINE void EndRecording() const { VK_CHECK(vkEndCommandBuffer(m_CommandBuffer), "Failed to end recording command buffer"); }

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

    FORCEINLINE void DrawIndirect(const VkBuffer& buffer, const VkDeviceSize offset, const uint32_t drawCount, const uint32_t stride)
    {
        vkCmdDrawIndirect(m_CommandBuffer, buffer, offset, drawCount, stride);
    }

    FORCEINLINE void BindVertexBuffers(const uint32_t firstBinding = 0, const uint32_t bindingCount = 1, VkBuffer* buffers = VK_NULL_HANDLE,
                                       VkDeviceSize* offsets = VK_NULL_HANDLE) const
    {
        GNT_ASSERT(buffers, "Invalid vertex buffer(s)!");
        vkCmdBindVertexBuffers(m_CommandBuffer, firstBinding, bindingCount, buffers, offsets);
    }

    FORCEINLINE void BindIndexBuffer(const VkBuffer& buffer, const VkDeviceSize offset = 0,
                                     VkIndexType indexType = VK_INDEX_TYPE_UINT32) const
    {
        vkCmdBindIndexBuffer(m_CommandBuffer, buffer, offset, indexType);
    }

  private:
    VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
    VkCommandBufferLevel m_Level    = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    friend class VulkanCommandPool;
};

}  // namespace Gauntlet