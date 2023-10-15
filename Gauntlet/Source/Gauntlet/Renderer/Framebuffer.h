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

// Always specify depth attachment as last element.
struct FramebufferAttachmentSpecification
{
  public:
    FramebufferAttachmentSpecification() = default;

    ETextureFilter Filter = ETextureFilter::LINEAR;
    ETextureWrap Wrap     = ETextureWrap::REPEAT;
    EImageFormat Format   = EImageFormat::NONE;
    glm::vec4 ClearColor  = glm::vec4(1.0f);
    ELoadOp LoadOp        = ELoadOp::CLEAR;
    EStoreOp StoreOp      = EStoreOp::STORE;
};

struct FramebufferAttachment
{
    std::array<Ref<Image>, FRAMES_IN_FLIGHT> Attachments;  // Per-frame
    FramebufferAttachmentSpecification Specification;
};

struct FramebufferSpecification
{
  public:
    // Always specify depth attachment as last element.
    std::vector<FramebufferAttachmentSpecification> Attachments;
    std::vector<FramebufferAttachment> ExistingAttachments;

    uint32_t Width   = 0;
    uint32_t Height  = 0;
    std::string Name = "None";
};

class Framebuffer : private Uncopyable, private Unmovable
{
  public:
    FORCEINLINE virtual FramebufferSpecification& GetSpecification() = 0;
    static Ref<Framebuffer> Create(const FramebufferSpecification& framebufferSpecification);

    virtual void Destroy()                               = 0;
    virtual void Resize(uint32_t width, uint32_t height) = 0;

    virtual void SetDepthStencilClearColor(const float depth, const uint32_t stencil)    = 0;
    FORCEINLINE virtual const std::vector<FramebufferAttachment>& GetAttachments() const = 0;

    virtual const uint32_t GetWidth() const  = 0;
    virtual const uint32_t GetHeight() const = 0;
};

}  // namespace Gauntlet