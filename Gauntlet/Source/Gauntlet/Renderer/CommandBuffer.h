#pragma once

#include "Gauntlet/Core/Core.h"

namespace Gauntlet
{

enum class ECommandBufferType : uint8_t
{
    COMMAND_BUFFER_TYPE_GRAPHICS = 0,
    COMMAND_BUFFER_TYPE_COMPUTE  = 1,
    COMMAND_BUFFER_TYPE_TRANSFER
};

class CommandBuffer : private Uncopyable, private Unmovable
{
  public:
    CommandBuffer()  = default;
    ~CommandBuffer() = default;

    virtual void BeginRecording(const void* inheritanceInfo = nullptr) = 0;
    virtual void EndRecording()                                        = 0;

    virtual void BeginTimestamp(bool bStatisticsQuery = false) = 0;
    virtual void EndTimestamp(bool bStatisticsQuery = false)   = 0;

    virtual const std::vector<size_t>& GetTimestampResults()  = 0;
    virtual const std::vector<size_t>& GetStatisticsResults() = 0;

    virtual void Submit()  = 0;
    virtual void Destroy() = 0;

    static Ref<CommandBuffer> Create(ECommandBufferType type);
};

}  // namespace Gauntlet