#pragma once

#include "Eclipse/Renderer/Mesh.h"
#include "VulkanBuffer.h"

#include "Eclipse/Renderer/CoreRendererStructs.h"

namespace Eclipse
{

class VulkanMesh final : public Mesh
{
  public:
    VulkanMesh(const std::string_view& InFilePath) : Mesh(InFilePath) {}
    VulkanMesh(const BufferInfo& InBufferInfo) : Mesh(InBufferInfo) {}
    ~VulkanMesh() = default;

    void Destroy() final override { Mesh::Destroy(); }

  private:
};

}  // namespace Eclipse