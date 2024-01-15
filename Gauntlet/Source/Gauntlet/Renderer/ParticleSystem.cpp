#include <GauntletPCH.h>
#include "ParticleSystem.h"

#include "Renderer.h"
#include "Pipeline.h"
#include "Buffer.h"
#include "CommandBuffer.h"
#include "Framebuffer.h"

#include "Gauntlet/Core/Random.h"
#include "Gauntlet/Core/Application.h"

namespace Gauntlet
{

// TODO: Make ParticleEmitter component?
ParticleSystem::ParticleSystem()
{
    std::vector<Particle> particles(m_BaseParticleCount);
    for (auto& particle : particles)
    {
        const float r     = 0.25f * sqrt(Random::GetInRange0To1());
        const float theta = Random::GetInRange0To1() * 2 * 3.14159265358979323846;
        const float x     = r * cos(theta);
        const float y     = r * sin(theta);
        const float z     = r;

        particle.position = glm::vec3(x, y, z);
        particle.velocity = glm::normalize(particle.position) * 0.00025f;
        particle.color    = glm::vec4(Random::GetInRange0To1(), Random::GetInRange0To1(), Random::GetInRange0To1(), 1.0f);
    }

    BufferSpecification ssboVertBufferInfo = {};
    ssboVertBufferInfo.Usage = EBufferUsageFlags::STORAGE_BUFFER | EBufferUsageFlags::VERTEX_BUFFER | EBufferUsageFlags::TRANSFER_DST;
    for (auto& buffer : m_SSBOVertexBuffer)
    {
        buffer = StorageBuffer::Create(ssboVertBufferInfo);
        buffer->SetData(particles.data(), particles.size() * sizeof(particles[0]));
    }

    PipelineSpecification psComputePipelineSpec = {};
    psComputePipelineSpec.Name                  = "GPU-Based_Compute_ParticleSystem";
    psComputePipelineSpec.Shader                = ShaderLibrary::Load("ParticleSystem");
    psComputePipelineSpec.PipelineType          = EPipelineType::PIPELINE_TYPE_COMPUTE;

    m_ComputePipeline = Pipeline::Create(psComputePipelineSpec);

    PipelineSpecification psRenderingPipelineSpec = {};
    psRenderingPipelineSpec.Name                  = "GPU-Based_Rendering_ParticleSystem";
    psRenderingPipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
    psRenderingPipelineSpec.PrimitiveTopology     = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_POINT_LIST;
    psRenderingPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
    psRenderingPipelineSpec.CullMode              = ECullMode::CULL_MODE_BACK;
    psRenderingPipelineSpec.Shader                = ShaderLibrary::Get("ParticleSystem");
    psRenderingPipelineSpec.PipelineType          = EPipelineType::PIPELINE_TYPE_GRAPHICS;
    psRenderingPipelineSpec.TargetFramebuffer     = Renderer::GetStorageData().GeometryFramebuffer;
    psRenderingPipelineSpec.DepthCompareOp        = ECompareOp::COMPARE_OP_LESS;
    psRenderingPipelineSpec.bDepthTest            = true;
    psRenderingPipelineSpec.bDepthWrite           = true;

    m_RenderingPipeline = Pipeline::Create(psRenderingPipelineSpec);
}

void ParticleSystem::Destroy()
{
    for (auto& Buffer : m_SSBOVertexBuffer)
        Buffer->Destroy();

    m_RenderingPipeline->Destroy();
    m_ComputePipeline->Destroy();
}

void ParticleSystem::OnUpdate()
{
    m_ComputePipeline->GetSpecification().Shader->Set(
        "s_ParticleSSBOIn", m_SSBOVertexBuffer[(GraphicsContext::Get().GetCurrentFrameIndex() - 1) % FRAMES_IN_FLIGHT]);

    m_ComputePipeline->GetSpecification().Shader->Set("s_ParticleSSBOOut",
                                                      m_SSBOVertexBuffer[GraphicsContext::Get().GetCurrentFrameIndex()]);
}

void ParticleSystem::OnCompute(uint32_t particleCount)
{
    m_CurrentParticleCount = particleCount;

    Ref<CommandBuffer> computeCommandBuffer = CommandBuffer::Create(ECommandBufferType::COMMAND_BUFFER_TYPE_COMPUTE);
    computeCommandBuffer->BeginRecording(true);
    computeCommandBuffer->BeginDebugLabel("GPU-Based Particle System", glm::vec4(0, 1, 0, 1));

    computeCommandBuffer->BeginTimestamp(true);
    computeCommandBuffer->BeginTimestamp();

    struct PushConstants
    {
        float DeltaTime;
        uint32_t ParticleCount;
    } u_ParticleSystemData;

    u_ParticleSystemData.ParticleCount = m_CurrentParticleCount;
    u_ParticleSystemData.DeltaTime     = Application::Get().GetDeltaTime();
    Renderer::Dispatch(computeCommandBuffer, m_ComputePipeline, &u_ParticleSystemData, m_CurrentParticleCount / 256, 1, 1);

    computeCommandBuffer->EndTimestamp();
    computeCommandBuffer->EndTimestamp(true);

    computeCommandBuffer->EndDebugLabel();
    computeCommandBuffer->EndRecording();
    computeCommandBuffer->Submit();

    Renderer::GetStats().PipelineStatisticsResults[10] += computeCommandBuffer->GetPipelineStatisticsResults()[0];

    auto& timestampResults = computeCommandBuffer->GetTimestampResults();
    const float time =
        static_cast<float>(timestampResults[1] - timestampResults[0]) * GraphicsContext::Get().GetTimestampPeriod() / 1000000.0f;
    const std::string str = "ParticleCompute Pass: " + std::to_string(time) + " (ms)";
    Renderer::GetStats().PassStatistsics.push_back(str);
}

void ParticleSystem::OnRender(void* pushConstants)
{
    GNT_ASSERT(m_RenderingPipeline->GetSpecification().TargetFramebuffer[GraphicsContext::Get().GetCurrentFrameIndex()],
               "Nowhere to render particle system!");

    Ref<CommandBuffer> renderCommandBuffer = CommandBuffer::Create(ECommandBufferType::COMMAND_BUFFER_TYPE_GRAPHICS);
    renderCommandBuffer->BeginRecording(true);
    renderCommandBuffer->BeginDebugLabel("GPU Rendering Computed Particles", glm::vec4(0.5f, 0.95f, 0.0f, 1.0f));
    renderCommandBuffer->BeginTimestamp();

    m_RenderingPipeline->GetSpecification().TargetFramebuffer[GraphicsContext::Get().GetCurrentFrameIndex()]->BeginPass(
        renderCommandBuffer);

    Renderer::SubmitParticleSystem(renderCommandBuffer, m_RenderingPipeline,
                                   m_SSBOVertexBuffer[GraphicsContext::Get().GetCurrentFrameIndex()], m_CurrentParticleCount,
                                   pushConstants);

    m_RenderingPipeline->GetSpecification().TargetFramebuffer[GraphicsContext::Get().GetCurrentFrameIndex()]->EndPass(renderCommandBuffer);

    renderCommandBuffer->EndTimestamp();
    renderCommandBuffer->EndDebugLabel();
    renderCommandBuffer->EndRecording();
    renderCommandBuffer->Submit();

    auto& timestampResults = renderCommandBuffer->GetTimestampResults();
    const float time =
        static_cast<float>(timestampResults[1] - timestampResults[0]) * GraphicsContext::Get().GetTimestampPeriod() / 1000000.0f;
    const std::string str = "ParticleRender Pass: " + std::to_string(time) + " (ms)";
    Renderer::GetStats().PassStatistsics.push_back(str);
}

}  // namespace Gauntlet