#include "GauntletPCH.h"
#include "Pipeline.h"

#include "RendererAPI.h"
#include "Gauntlet/Platform/Vulkan/VulkanPipeline.h"

namespace Gauntlet
{
Ref<Pipeline> Pipeline::Create(const PipelineSpecification& pipelineSpec)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan: return MakeRef<VulkanPipeline>(pipelineSpec);
        case RendererAPI::EAPI::None:
        {
            GNT_ASSERT(false, "RendererAPI::None!");
            return nullptr;
        }
    }

    GNT_ASSERT(false, "Unknown RHI!");
    return nullptr;
}

}  // namespace Gauntlet