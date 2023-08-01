#pragma once

#include "Eclipse/Core/Core.h"
#include "VulkanCore.h"
#include "VulkanUtility.h"

#include <volk/volk.h>

#include <glm/glm.hpp>

namespace Eclipse
{

class VulkanPipeline;

class VulkanCommandBuffer final /*: private Uncopyable, private Unmovable*/
{
  public:
    VulkanCommandBuffer()  = default;
    ~VulkanCommandBuffer() = default;

    FORCEINLINE const auto& Get() const { return m_CommandBuffer; }
    FORCEINLINE auto& Get() { return m_CommandBuffer; }

    FORCEINLINE void Reset() const { VK_CHECK(vkResetCommandBuffer(m_CommandBuffer, 0), "Failed to reset command buffer!"); }

    void BeginDebugLabel(const char* InCommandBufferLabelName = "NONAME", const glm::vec4& InLabelColor = glm::vec4(1.0f)) const;
    FORCEINLINE void EndDebugLabel() const
    {
        if (bEnableValidationLayers) vkCmdEndDebugUtilsLabelEXT(m_CommandBuffer);
    }

    void BeginRecording() const;
    FORCEINLINE void EndRecording() const { VK_CHECK(vkEndCommandBuffer(m_CommandBuffer), "Failed to end recording command buffer"); }

    FORCEINLINE void BindPushConstants(VkPipelineLayout InPipelineLayout, const VkShaderStageFlags InShaderStageFlags,
                                       const uint32_t InOffset, const uint32_t InSize, const void* InValues = VK_NULL_HANDLE) const
    {
        vkCmdPushConstants(m_CommandBuffer, InPipelineLayout, InShaderStageFlags, InOffset, InSize, InValues);
    }

    FORCEINLINE void BindDescriptorSets(VkPipelineLayout InPipelineLayout, const uint32_t InFirstSet = 0,
                                        const uint32_t InDescriptorCount = 0, VkDescriptorSet* InDescriptorSets = VK_NULL_HANDLE,
                                        VkPipelineBindPoint InPipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        const uint32_t InDynamicOffsetCount = 0, uint32_t* InDynamicOffsets = nullptr) const
    {
        vkCmdBindDescriptorSets(m_CommandBuffer, InPipelineBindPoint, InPipelineLayout, InFirstSet, InDescriptorCount, InDescriptorSets,
                                InDynamicOffsetCount, InDynamicOffsets);
    }

    void BindPipeline(const Ref<VulkanPipeline>& InPipeline,
                      VkPipelineBindPoint InPipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS) const;

    FORCEINLINE void DrawIndexed(const uint32_t InIndexCount, const uint32_t InInstanceCount = 1, const uint32_t InFirstIndex = 0,
                                 const int32_t InVertexOffset = 0, const uint32_t InFirstInstance = 0) const
    {
        vkCmdDrawIndexed(m_CommandBuffer, InIndexCount, InInstanceCount, InFirstIndex, InVertexOffset, InFirstInstance);
    }

    FORCEINLINE void Draw(const uint32_t InVertexCount, const uint32_t InInstanceCount = 1, const uint32_t InFirstVertex = 0,
                          const uint32_t InFirstInstance = 0) const
    {
        vkCmdDraw(m_CommandBuffer, InVertexCount, InInstanceCount, InFirstVertex, InFirstInstance);
    }

    FORCEINLINE void BindVertexBuffers(const uint32_t InFirstBinding = 0, const uint32_t InBindingCount = 1,
                                       VkBuffer* InBuffers = VK_NULL_HANDLE, VkDeviceSize* InOffsets = VK_NULL_HANDLE) const
    {
        vkCmdBindVertexBuffers(m_CommandBuffer, InFirstBinding, InBindingCount, InBuffers, InOffsets);
    }

    FORCEINLINE void BindIndexBuffer(const VkBuffer& InBuffer, const VkDeviceSize InOffset, VkIndexType InIndexType) const
    {
        vkCmdBindIndexBuffer(m_CommandBuffer, InBuffer, InOffset, InIndexType);
    }

  private:
    VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
};

}  // namespace Eclipse