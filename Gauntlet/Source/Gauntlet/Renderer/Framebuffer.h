#pragma once

#include "Gauntlet/Core/Core.h"
#include "Image.h"

#include <glm/glm.hpp>

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
  public:
    FramebufferAttachmentSpecification() = default;

    ETextureFilter Filter = ETextureFilter::LINEAR;
    ETextureWrap Wrap     = ETextureWrap::REPEAT;
    EImageFormat Format   = EImageFormat::NONE;
    glm::vec4 ClearColor  = glm::vec4(1.0f);
    ELoadOp LoadOp        = ELoadOp::CLEAR;
    EStoreOp StoreOp      = EStoreOp::STORE;
};

struct FramebufferSpecification
{
  public:
    std::vector<FramebufferAttachmentSpecification> Attachments;
    std::vector<Ref<Image>> AliveAttachments;

    uint32_t Width   = 0;
    uint32_t Height  = 0;
    std::string Name = "None";
};

class Framebuffer : private Uncopyable, private Unmovable
{
  public:
    FORCEINLINE virtual FramebufferSpecification& GetSpec() = 0;
    static Ref<Framebuffer> Create(const FramebufferSpecification& framebufferSpecification);

    virtual void Destroy()                               = 0;
    virtual void Resize(uint32_t width, uint32_t height) = 0;

    virtual const uint32_t GetWidth() const  = 0;
    virtual const uint32_t GetHeight() const = 0;

  private:
};

}  // namespace Gauntlet