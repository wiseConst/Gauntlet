#pragma once

#include "Gauntlet/Core/Core.h"
#include "Gauntlet/Core/Math.h"
#include <array>

namespace Gauntlet
{
class Pipeline;
class Framebuffer;
class StorageBuffer;

struct Particle
{
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 color;
};

class ParticleSystem final : private Uncopyable, private Unmovable
{
  public:
    ParticleSystem();
    ~ParticleSystem() = default;

    void Destroy();

    void OnUpdate();
    void OnCompute(uint32_t particleCount);
    void OnRender(void* pushConstants = nullptr);

    FORCEINLINE const auto& GetRenderingPipeline() const { return m_RenderingPipeline; }
    FORCEINLINE const auto& GetSSBOBuffers() const { return m_SSBOVertexBuffer; }

  private:
    Ref<Pipeline> m_ComputePipeline = nullptr;
    std::array<Ref<StorageBuffer>, FRAMES_IN_FLIGHT> m_SSBOVertexBuffer;
    Ref<Pipeline> m_RenderingPipeline = nullptr;

    uint32_t m_BaseParticleCount    = 2048;
    uint32_t m_CurrentParticleCount = 0;
};

}  // namespace Gauntlet