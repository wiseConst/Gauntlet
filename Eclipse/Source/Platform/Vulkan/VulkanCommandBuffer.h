#pragma once

#include "Eclipse/Core/Core.h"
#include "VulkanCore.h"
#include <volk/volk.h>

namespace Eclipse
{

/*
    One very important part with command buffers is that they can be recorded in parallel.
    You can record different command buffers from different threads safely.
    To do that, you need to have 1 VkCommandPool and 1 VkCommandBuffer per thread(minimum),
    and make sure that each thread only uses their own command buffers & pools.
    Once that is done, it’s possible to submit the command buffers in one of the threads.
    vkQueueSubmit is not thread - safe, only one thread can push the commands at a time.
*/
class VulkanCommandBuffer final /*: private Uncopyable, private Unmovable*/
{
  public:
    FORCEINLINE void Reset() const { VK_CHECK(vkResetCommandBuffer(m_CommandBuffer, 0), "Failed to reset command buffer!"); }

    void BeginRecording() const;
    FORCEINLINE void EndRecording() const { VK_CHECK(vkEndCommandBuffer(m_CommandBuffer), "Failed to end recording command buffer"); }

    FORCEINLINE const auto& Get() const { return m_CommandBuffer; }
    FORCEINLINE auto& Get() { return m_CommandBuffer; }

  private:
    VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
};

}  // namespace Eclipse