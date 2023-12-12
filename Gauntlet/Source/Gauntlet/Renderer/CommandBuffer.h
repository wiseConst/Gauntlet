#pragma once

#include "Gauntlet/Core/Core.h"
#include "Gauntlet/Core/Math.h"

namespace Gauntlet
{

enum class ECommandBufferType : uint8_t
{
    COMMAND_BUFFER_TYPE_GRAPHICS = 0,
    COMMAND_BUFFER_TYPE_COMPUTE  = 1,
    COMMAND_BUFFER_TYPE_TRANSFER
};

enum class ECommandBufferLevel : uint8_t
{
    COMMAND_BUFFER_LEVEL_PRIMARY   = 0,
    COMMAND_BUFFER_LEVEL_SECONDARY = 1
};

class CommandBuffer : private Uncopyable, private Unmovable
{
  public:
    CommandBuffer()          = default;
    virtual ~CommandBuffer() = default;

    virtual ECommandBufferLevel GetLevel() const = 0;

    virtual void BeginRecording(bool bOneTimeSubmit = false, const void* inheritanceInfo = nullptr) = 0;
    virtual void EndRecording()                                                                     = 0;

    virtual void BeginTimestamp(bool bStatisticsQuery = false) = 0;
    virtual void EndTimestamp(bool bStatisticsQuery = false)   = 0;

    virtual void BeginDebugLabel(const char* commandBufferLabelName = "NONAME", const glm::vec4& labelColor = glm::vec4(1.0f)) const = 0;
    virtual void EndDebugLabel() const                                                                                               = 0;

    virtual const std::vector<size_t>& GetTimestampResults() const              = 0;
    virtual const std::vector<size_t>& GetPipelineStatisticsResults() const     = 0;
    virtual const std::vector<std::string> GetPipelineStatisticsStrings() const = 0;

    virtual void Submit(bool bWaitAfterSubmit = true) = 0;
    virtual void Reset()                              = 0;
    virtual void Destroy()                            = 0;

    static Ref<CommandBuffer> Create(ECommandBufferType type,
                                     ECommandBufferLevel level = ECommandBufferLevel::COMMAND_BUFFER_LEVEL_PRIMARY);
};

}  // namespace Gauntlet