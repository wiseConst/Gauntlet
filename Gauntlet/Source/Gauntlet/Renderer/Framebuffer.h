#pragma once

#include "Gauntlet/Core/Core.h"
#include "Image.h"

#include "Gauntlet/Core/Math.h"

namespace Gauntlet
{

enum class ELoadOp : uint8_t
{
    CLEAR = 0,
    LOAD,
    DONT_CARE
};

enum class EStoreOp : uint8_t
{
    STORE = 0,
    DONT_CARE
};

struct FramebufferAttachmentSpecification
{
    EImageFormat Format   = EImageFormat::NONE;
    ETextureFilter Filter = ETextureFilter::LINEAR;
    ETextureWrap Wrap     = ETextureWrap::REPEAT;
    ELoadOp LoadOp        = ELoadOp::CLEAR;
    EStoreOp StoreOp      = EStoreOp::STORE;
    glm::vec4 ClearColor  = glm::vec4(1.0f);
};

struct FramebufferAttachment
{
    Ref<Image> Attachment = nullptr;
    FramebufferAttachmentSpecification Specification;
};

struct FramebufferSpecification
{
  public:
    std::vector<FramebufferAttachmentSpecification> Attachments;
    std::vector<FramebufferAttachment> ExistingAttachments;

    // These are in case you have existing attachments
    ELoadOp LoadOp   = ELoadOp::CLEAR;
    EStoreOp StoreOp = EStoreOp::STORE;

    uint32_t Width   = 0;
    uint32_t Height  = 0;
    std::string Name = "None";
};

class CommandBuffer;

class Framebuffer : private Uncopyable, private Unmovable
{
  public:
    Framebuffer()          = default;
    virtual ~Framebuffer() = default;

    FORCEINLINE virtual FramebufferSpecification& GetSpecification() = 0;
    static Ref<Framebuffer> Create(const FramebufferSpecification& framebufferSpecification);

    virtual void Destroy()                               = 0;
    virtual void Resize(uint32_t width, uint32_t height) = 0;

    virtual void BeginPass(const Ref<CommandBuffer>& commandBuffer) = 0;
    virtual void EndPass(const Ref<CommandBuffer>& commandBuffer)   = 0;

    FORCEINLINE virtual const std::vector<FramebufferAttachment>& GetAttachments() const = 0;

    virtual const uint32_t GetWidth() const  = 0;
    virtual const uint32_t GetHeight() const = 0;
};

}  // namespace Gauntlet