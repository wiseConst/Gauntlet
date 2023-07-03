#pragma once

#include "Eclipse/Renderer/Mesh.h"
#include "VulkanBuffer.h"

namespace Eclipse
{

// PushConstants have a minimum size of 128 bytes, which is enough
// for common things like a 4x4 matrix and a few extra parameters.
struct alignas(16) MeshPushConstants
{
    glm::mat4 RenderMatrix;
    glm::vec4 Data;
};

class VulkanMesh final : public Mesh
{
  public:
    VulkanMesh(const std::string_view& InFilePath) : Mesh(InFilePath) {}
    ~VulkanMesh() = default;

    void Destroy() final override { Mesh::Destroy(); }

  private:
};

}  // namespace Eclipse