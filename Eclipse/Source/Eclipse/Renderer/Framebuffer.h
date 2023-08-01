#pragma once

#include "Eclipse/Core/Core.h"

#include <glm/glm.hpp>

namespace Eclipse
{

// Currently unused class

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

struct FramebufferSpecification
{
  public:
    glm::vec4 ClearColor = glm::vec4(1.0f);

    bool bHasDepth{false};
    bool bIsSwapchainTarget{false};
    ELoadOp LoadOp   = ELoadOp::CLEAR;
    EStoreOp StoreOp = EStoreOp::STORE;

    uint32_t Width  = 0;
    uint32_t Height = 0;
};

class Framebuffer : private Uncopyable, private Unmovable
{
  public:
    FORCEINLINE virtual FramebufferSpecification& GetSpec() = 0;
    static Ref<Framebuffer> Create(const FramebufferSpecification& InFramebufferSpecification);

    virtual void Destroy() = 0;

  private:
};

}  // namespace Eclipse