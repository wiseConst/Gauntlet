#include "GauntletPCH.h"
#include "VulkanRenderer.h"

#include "Gauntlet/Renderer/Camera/PerspectiveCamera.h"
#include "Gauntlet/Core/JobSystem.h"

#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"
#include "VulkanPipeline.h"
#include "VulkanFramebuffer.h"

#include "VulkanRenderer2D.h"
#include "VulkanUtility.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "VulkanTextureCube.h"
#include "VulkanBuffer.h"
#include "VulkanMaterial.h"

#include "Gauntlet/Renderer/Skybox.h"
#include "Gauntlet/Renderer/Mesh.h"

namespace Gauntlet
{

VulkanRenderer::VulkanRenderer() : m_Context((VulkanContext&)VulkanContext::Get())
{
    Create();
}

void VulkanRenderer::Create()
{
    s_Data.MeshVertexBufferLayout = {
        {EShaderDataType::Vec3, "InPosition"},  //
        {EShaderDataType::Vec3, "InNormal"},    //
        {EShaderDataType::Vec4, "InColor"},     //
        {EShaderDataType::Vec2, "InTexCoord"},  //
        {EShaderDataType::Vec3, "InTangent"},   //
        {EShaderDataType::Vec3, "InBitangent"}  //
    };

    const auto TextureDescriptorSetLayoutBinding =
        Utility::GetDescriptorSetLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutCreateInfo ImageDescriptorSetLayoutCreateInfo =
        Utility::GetDescriptorSetLayoutCreateInfo(1, &TextureDescriptorSetLayoutBinding);
    VK_CHECK(vkCreateDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), &ImageDescriptorSetLayoutCreateInfo, nullptr,
                                         &s_Data.ImageDescriptorSetLayout),
             "Failed to create texture(UI) descriptor set layout!");

    {
        const uint32_t WhiteTexutreData = 0xffffffff;
        s_Data.MeshWhiteTexture         = MakeRef<VulkanTexture2D>(&WhiteTexutreData, sizeof(WhiteTexutreData), 1, 1);
    }

    {
        FramebufferSpecification shadowMapFramebufferSpec = {};

        FramebufferAttachmentSpecification shadowMapAttachment = {};
        shadowMapAttachment.ClearColor                         = glm::vec4(1.0f, glm::vec3(0.0f));
        shadowMapAttachment.Format                             = EImageFormat::DEPTH32F;
        shadowMapAttachment.LoadOp                             = ELoadOp::CLEAR;
        shadowMapAttachment.StoreOp                            = EStoreOp::STORE;
        shadowMapAttachment.Filter                             = ETextureFilter::NEAREST;
        shadowMapAttachment.Wrap                               = ETextureWrap::CLAMP;

        shadowMapFramebufferSpec.Attachments = {shadowMapAttachment};
        shadowMapFramebufferSpec.Name        = "ShadowMap";
        shadowMapFramebufferSpec.Width       = 4096;
        shadowMapFramebufferSpec.Height      = 4096;

        s_Data.ShadowMapFramebuffer = MakeRef<VulkanFramebuffer>(shadowMapFramebufferSpec);
    }

    {
        FramebufferSpecification geometryFramebufferSpec = {};

        FramebufferAttachmentSpecification attachment = {};
        attachment.LoadOp                             = ELoadOp::LOAD;
        attachment.StoreOp                            = EStoreOp::STORE;
        attachment.ClearColor                         = glm::vec4(glm::vec3(0.0f), 1.0f);

        // Position
        attachment.Format = EImageFormat::RGBA16F;
        geometryFramebufferSpec.Attachments.push_back(attachment);

        // Normal
        attachment.Format = EImageFormat::RGBA16F;
        geometryFramebufferSpec.Attachments.push_back(attachment);

        // Albedo
        attachment.ClearColor = glm::vec4(glm::vec3(0.1f), 1.0f);
        attachment.Format     = EImageFormat::RGBA;
        geometryFramebufferSpec.Attachments.push_back(attachment);

        // Depth
        attachment.ClearColor = glm::vec4(1.0f, glm::vec3(0.0f));
        attachment.Format     = EImageFormat::DEPTH32F;
        geometryFramebufferSpec.Attachments.push_back(attachment);

        geometryFramebufferSpec.Name = "Geometry";

        s_Data.GeometryFramebuffer = MakeRef<VulkanFramebuffer>(geometryFramebufferSpec);
    }

    {
        FramebufferSpecification setupFramebufferSpec = {};
        setupFramebufferSpec.ExistingAttachments      = s_Data.GeometryFramebuffer->GetAttachments();
        setupFramebufferSpec.Name                     = "Setup";

        s_Data.SetupFramebuffer = MakeRef<VulkanFramebuffer>(setupFramebufferSpec);
    }

    {
        FramebufferSpecification lightingFramebufferSpec = {};

        FramebufferAttachmentSpecification attachment = {};

        attachment.ClearColor = glm::vec4(glm::vec3(0.1f), 1.0f);
        attachment.Format     = EImageFormat::RGBA;
        attachment.LoadOp     = ELoadOp::LOAD;
        attachment.StoreOp    = EStoreOp::STORE;
        lightingFramebufferSpec.Attachments.push_back(attachment);

        lightingFramebufferSpec.Name = "FinalPass";
        s_Data.LightingFramebuffer   = MakeRef<VulkanFramebuffer>(lightingFramebufferSpec);
    }

    {
        constexpr auto DiffuseTextureBinding =
            Utility::GetDescriptorSetLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        constexpr auto NormalTextureBinding =
            Utility::GetDescriptorSetLayoutBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        constexpr auto EmissiveTextureBinding =
            Utility::GetDescriptorSetLayoutBinding(2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        constexpr auto EnvironmentMapBinding =
            Utility::GetDescriptorSetLayoutBinding(3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        constexpr auto CameraDataBufferBinding =
            Utility::GetDescriptorSetLayoutBinding(4, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
        constexpr auto PhongModelBufferBinding =
            Utility::GetDescriptorSetLayoutBinding(5, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
        constexpr auto ShadowMapBinding =
            Utility::GetDescriptorSetLayoutBinding(6, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        constexpr auto ShadowsBufferBinding =
            Utility::GetDescriptorSetLayoutBinding(7, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);

        std::vector<VkDescriptorSetLayoutBinding> Bindings = {DiffuseTextureBinding, NormalTextureBinding,    EmissiveTextureBinding,
                                                              EnvironmentMapBinding, CameraDataBufferBinding, PhongModelBufferBinding,
                                                              ShadowMapBinding,      ShadowsBufferBinding};
        auto MeshDescriptorSetLayoutCreateInfo =
            Utility::GetDescriptorSetLayoutCreateInfo(static_cast<uint32_t>(Bindings.size()), Bindings.data());
        VK_CHECK(vkCreateDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), &MeshDescriptorSetLayoutCreateInfo, nullptr,
                                             &s_Data.MeshDescriptorSetLayout),
                 "Failed to create mesh descriptor set layout!");
    }

    // Geometry pipeline
    {
        PipelineSpecification PipelineSpec = {};
        PipelineSpec.Name                  = "Geometry";
        PipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        PipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        PipelineSpec.PrimitiveTopology     = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        PipelineSpec.CullMode              = ECullMode::CULL_MODE_NONE;
        PipelineSpec.bDepthTest            = VK_TRUE;
        PipelineSpec.bDepthWrite           = VK_TRUE;
        PipelineSpec.DepthCompareOp        = VK_COMPARE_OP_LESS;
        PipelineSpec.TargetFramebuffer     = s_Data.GeometryFramebuffer;

        auto GeometryVertexShader   = Ref<VulkanShader>(new VulkanShader("Resources/Cached/Shaders/Mesh.vert.spv"));
        auto GeometryFragmentShader = Ref<VulkanShader>(new VulkanShader("Resources/Cached/Shaders/Mesh.frag.spv"));

        PipelineSpec.ShaderStages = {
            {GeometryVertexShader, EShaderStage::SHADER_STAGE_VERTEX},     //
            {GeometryFragmentShader, EShaderStage::SHADER_STAGE_FRAGMENT}  //
        };

        PipelineSpec.PushConstantRanges        = {Utility::GetPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(MeshPushConstants))};
        PipelineSpec.ShaderBindingDescriptions = {Utility::GetShaderBindingDescription(0, s_Data.MeshVertexBufferLayout.GetStride())};

        std::vector<VkVertexInputAttributeDescription> ShaderAttributeDescriptions;
        ShaderAttributeDescriptions.reserve(s_Data.MeshVertexBufferLayout.GetElements().size());
        for (uint32_t i = 0; i < ShaderAttributeDescriptions.capacity(); ++i)
        {
            const auto& CurrentBufferElement = s_Data.MeshVertexBufferLayout.GetElements()[i];
            ShaderAttributeDescriptions.emplace_back(i, 0, Utility::GauntletShaderDataTypeToVulkan(CurrentBufferElement.Type),
                                                     CurrentBufferElement.Offset);
        }
        PipelineSpec.ShaderAttributeDescriptions = ShaderAttributeDescriptions;

        PipelineSpec.DescriptorSetLayouts = {s_Data.MeshDescriptorSetLayout};
        s_Data.GeometryPipeline           = MakeRef<VulkanPipeline>(PipelineSpec);

        GeometryFragmentShader->Destroy();
        GeometryVertexShader->Destroy();
    }

    SetupSkybox();

    constexpr VkDeviceSize CameraDataBufferSize = sizeof(CameraDataBuffer);
    s_Data.UniformCameraDataBuffers.resize(FRAMES_IN_FLIGHT);
    s_Data.MappedUniformCameraDataBuffers.resize(FRAMES_IN_FLIGHT);

    constexpr VkDeviceSize PhongModelBufferSize = sizeof(LightingModelBuffer);
    s_Data.UniformPhongModelBuffers.resize(FRAMES_IN_FLIGHT);
    s_Data.MappedUniformPhongModelBuffers.resize(FRAMES_IN_FLIGHT);

    constexpr VkDeviceSize ShadowsBufferBufferSize = sizeof(ShadowsBuffer);
    s_Data.UniformShadowsBuffers.resize(FRAMES_IN_FLIGHT);
    s_Data.MappedUniformShadowsBuffers.resize(FRAMES_IN_FLIGHT);
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        // Camera
        BufferUtils::CreateBuffer(EBufferUsageFlags::UNIFORM_BUFFER, CameraDataBufferSize, s_Data.UniformCameraDataBuffers[i],
                                  VMA_MEMORY_USAGE_CPU_ONLY);

        s_Data.MappedUniformCameraDataBuffers[i] = m_Context.GetAllocator()->Map(s_Data.UniformCameraDataBuffers[i].Allocation);

        // Lighting
        BufferUtils::CreateBuffer(EBufferUsageFlags::UNIFORM_BUFFER, PhongModelBufferSize, s_Data.UniformPhongModelBuffers[i],
                                  VMA_MEMORY_USAGE_CPU_ONLY);

        s_Data.MappedUniformPhongModelBuffers[i] = m_Context.GetAllocator()->Map(s_Data.UniformPhongModelBuffers[i].Allocation);

        // Shadows
        BufferUtils::CreateBuffer(EBufferUsageFlags::UNIFORM_BUFFER, ShadowsBufferBufferSize, s_Data.UniformShadowsBuffers[i],
                                  VMA_MEMORY_USAGE_CPU_ONLY);

        s_Data.MappedUniformShadowsBuffers[i] = m_Context.GetAllocator()->Map(s_Data.UniformShadowsBuffers[i].Allocation);
    }

    // ShadowMap pipeline
    {
        auto DepthVertexShader   = Ref<VulkanShader>(new VulkanShader("Resources/Cached/Shaders/Depth.vert.spv"));
        auto DepthFragmentShader = Ref<VulkanShader>(new VulkanShader("Resources/Cached/Shaders/Depth.frag.spv"));

        PipelineSpecification PipelineSpec = {};
        PipelineSpec.Name                  = "ShadowMap";
        PipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        PipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        PipelineSpec.CullMode              = ECullMode::CULL_MODE_FRONT;  // BACK was
        PipelineSpec.bDepthWrite           = VK_TRUE;
        PipelineSpec.bDepthTest            = VK_TRUE;
        PipelineSpec.DepthCompareOp        = VK_COMPARE_OP_LESS;
        PipelineSpec.TargetFramebuffer     = s_Data.ShadowMapFramebuffer;
        PipelineSpec.PushConstantRanges    = {Utility::GetPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(LightPushConstants))};
        PipelineSpec.ShaderStages          = {{DepthVertexShader, EShaderStage::SHADER_STAGE_VERTEX},
                                     {DepthFragmentShader, EShaderStage::SHADER_STAGE_FRAGMENT}};

        PipelineSpec.ShaderBindingDescriptions = {Utility::GetShaderBindingDescription(0, s_Data.MeshVertexBufferLayout.GetStride())};
        PipelineSpec.ShaderAttributeDescriptions.emplace_back(0, 0, Utility::GauntletShaderDataTypeToVulkan(EShaderDataType::Vec3), 0);

        s_Data.ShadowMapPipeline = MakeRef<VulkanPipeline>(PipelineSpec);

        DepthVertexShader->Destroy();
        DepthFragmentShader->Destroy();
    }

    // Debug ShadowMap pipeline
    {
        auto DepthVertexShader   = Ref<VulkanShader>(new VulkanShader("Resources/Cached/Shaders/DebugShadowQuad.vert.spv"));
        auto DepthFragmentShader = Ref<VulkanShader>(new VulkanShader("Resources/Cached/Shaders/DebugShadowQuad.frag.spv"));

        PipelineSpecification PipelineSpec = {};
        PipelineSpec.Name                  = "DebugShadowMap";
        PipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        PipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        PipelineSpec.CullMode              = ECullMode::CULL_MODE_BACK;
        PipelineSpec.bDepthWrite           = VK_TRUE;
        PipelineSpec.bDepthTest            = VK_TRUE;
        PipelineSpec.DepthCompareOp        = VK_COMPARE_OP_LESS;
        PipelineSpec.TargetFramebuffer     = s_Data.GeometryFramebuffer;
        PipelineSpec.ShaderStages          = {{DepthVertexShader, EShaderStage::SHADER_STAGE_VERTEX},
                                     {DepthFragmentShader, EShaderStage::SHADER_STAGE_FRAGMENT}};

        auto DebugShadowMapDescriptorSetLayoutBinding =
            Utility::GetDescriptorSetLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        auto DebugShadowMapDescriptorSetLayoutCreateInfo =
            Utility::GetDescriptorSetLayoutCreateInfo(1, &DebugShadowMapDescriptorSetLayoutBinding);
        VK_CHECK(vkCreateDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), &DebugShadowMapDescriptorSetLayoutCreateInfo,
                                             VK_NULL_HANDLE, &s_Data.DebugShadowMapDescriptorSetLayout),
                 "Failed to create debug shadow map descriptor set layout!");
        PipelineSpec.DescriptorSetLayouts = {s_Data.DebugShadowMapDescriptorSetLayout};

        s_Data.DebugShadowMapPipeline = MakeRef<VulkanPipeline>(PipelineSpec);

        DepthVertexShader->Destroy();
        DepthFragmentShader->Destroy();

        GNT_ASSERT(
            m_Context.GetDescriptorAllocator()->Allocate(s_Data.DebugShadowMapDescriptorSet, s_Data.DebugShadowMapDescriptorSetLayout),
            "Failed to allocate debug shadow map descriptor set!");
        auto debugShadowMapWriteSet =
            Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, s_Data.DebugShadowMapDescriptorSet.Handle, 1,
                                           &s_Data.ShadowMapFramebuffer->GetDepthAttachment()->GetDescriptorInfo());
        vkUpdateDescriptorSets(m_Context.GetDevice()->GetLogicalDevice(), 1, &debugShadowMapWriteSet, 0, nullptr);
    }

    // Deffered pipeline
    {
        auto DefferedVertexShader   = Ref<VulkanShader>(new VulkanShader("Resources/Cached/Shaders/mrt.vert.spv"));
        auto DefferedFragmentShader = Ref<VulkanShader>(new VulkanShader("Resources/Cached/Shaders/mrt.frag.spv"));

        constexpr auto AlbedoBinding =
            Utility::GetDescriptorSetLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        constexpr auto NormalMapBinding =
            Utility::GetDescriptorSetLayoutBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        constexpr auto cameraBufferBinding =
            Utility::GetDescriptorSetLayoutBinding(2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);

        std::vector<VkDescriptorSetLayoutBinding> bindings = {AlbedoBinding, NormalMapBinding, cameraBufferBinding};
        auto DefferedDescriptorSetLayoutCreateInfo         = Utility::GetDescriptorSetLayoutCreateInfo(bindings.size(), bindings.data());
        VK_CHECK(vkCreateDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), &DefferedDescriptorSetLayoutCreateInfo, nullptr,
                                             &s_Data.DefferedDescriptorSetLayout),
                 "Failed to create deffered descriptor set layout!");

        PipelineSpecification PipelineSpec = {};
        PipelineSpec.Name                  = "Deffered";
        PipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        PipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        PipelineSpec.CullMode              = ECullMode::CULL_MODE_BACK;  // BACK was
        PipelineSpec.bDepthWrite           = VK_TRUE;
        PipelineSpec.bDepthTest            = VK_TRUE;
        PipelineSpec.DepthCompareOp        = VK_COMPARE_OP_LESS;
        PipelineSpec.TargetFramebuffer     = s_Data.GeometryFramebuffer;

        PipelineSpec.PushConstantRanges = {Utility::GetPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(MeshPushConstants))};
        PipelineSpec.ShaderStages       = {{DefferedVertexShader, EShaderStage::SHADER_STAGE_VERTEX},
                                     {DefferedFragmentShader, EShaderStage::SHADER_STAGE_FRAGMENT}};

        PipelineSpec.DescriptorSetLayouts = {s_Data.DefferedDescriptorSetLayout};

        // layout
        BufferLayout defferedLayout = {
            {EShaderDataType::Vec3, "InPosition"}, {EShaderDataType::Vec3, "InNormal"},  {EShaderDataType::Vec4, "InColor"},
            {EShaderDataType::Vec2, "InTexCoord"}, {EShaderDataType::Vec3, "InTangent"}, {EShaderDataType::Vec3, "InBitangent"},
        };
        PipelineSpec.ShaderBindingDescriptions = {Utility::GetShaderBindingDescription(0, defferedLayout.GetStride())};
        for (uint32_t i = 0; i < defferedLayout.GetElements().size(); ++i)
        {
            PipelineSpec.ShaderAttributeDescriptions.emplace_back(
                i, 0, Utility::GauntletShaderDataTypeToVulkan(defferedLayout.GetElements()[i].Type),
                defferedLayout.GetElements()[i].Offset);
        }

        s_Data.DefferedPipeline = MakeRef<VulkanPipeline>(PipelineSpec);

        DefferedVertexShader->Destroy();
        DefferedFragmentShader->Destroy();
    }

    // Lighting(using mrt) pipeline
    {
        auto LightingVertexShader   = Ref<VulkanShader>(new VulkanShader("Resources/Cached/Shaders/Deferred.vert.spv"));
        auto LightingFragmentShader = Ref<VulkanShader>(new VulkanShader("Resources/Cached/Shaders/Deferred.frag.spv"));

        constexpr auto PositionTextureBinding =
            Utility::GetDescriptorSetLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        constexpr auto NormalTextureBinding =
            Utility::GetDescriptorSetLayoutBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        constexpr auto AlbedoTextureBinding =
            Utility::GetDescriptorSetLayoutBinding(2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        constexpr auto LightingBufferBinding =
            Utility::GetDescriptorSetLayoutBinding(3, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);

        std::vector<VkDescriptorSetLayoutBinding> bindings = {PositionTextureBinding, NormalTextureBinding, AlbedoTextureBinding,
                                                              LightingBufferBinding};
        auto LightingDescriptorSetLayoutCreateInfo         = Utility::GetDescriptorSetLayoutCreateInfo(bindings.size(), bindings.data());
        VK_CHECK(vkCreateDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), &LightingDescriptorSetLayoutCreateInfo, nullptr,
                                             &s_Data.LightingDescriptorSetLayout),
                 "Failed to create lighting descriptor set layout!");

        GNT_ASSERT(m_Context.GetDescriptorAllocator()->Allocate(s_Data.LightingSet, s_Data.LightingDescriptorSetLayout),
                   "Failed to allocate lighting descriptor set!");

        for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
        {
            std::vector<VkWriteDescriptorSet> writes;

            for (uint32_t k = 0; k < s_Data.GeometryFramebuffer->GetAttachments().size() - 1; ++k)
            {
                Ref<VulkanImage> vulkanImage =
                    static_pointer_cast<VulkanImage>(s_Data.GeometryFramebuffer->GetAttachments()[k].Attachments[i]);
                auto textureWriteSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, k,
                                                                      s_Data.LightingSet.Handle, 1, &vulkanImage->GetDescriptorInfo());
                writes.push_back(textureWriteSet);
            }

            VkDescriptorBufferInfo LightingModelBufferInfo = {};
            LightingModelBufferInfo.buffer                 = s_Data.UniformPhongModelBuffers[i].Buffer;
            LightingModelBufferInfo.range                  = sizeof(LightingModelBuffer);
            LightingModelBufferInfo.offset                 = 0;

            // PhongModel
            auto PhongModelBufferWriteSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3, s_Data.LightingSet.Handle,
                                                                           1, &LightingModelBufferInfo);

            writes.push_back(PhongModelBufferWriteSet);

            vkUpdateDescriptorSets(m_Context.GetDevice()->GetLogicalDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0,
                                   nullptr);
        }

        PipelineSpecification PipelineSpec = {};
        PipelineSpec.Name                  = "Lighting";
        PipelineSpec.TargetFramebuffer     = s_Data.LightingFramebuffer;
        PipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        PipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;

        PipelineSpec.PushConstantRanges = {Utility::GetPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(MeshPushConstants))};
        PipelineSpec.ShaderStages       = {{LightingVertexShader, EShaderStage::SHADER_STAGE_VERTEX},
                                     {LightingFragmentShader, EShaderStage::SHADER_STAGE_FRAGMENT}};

        PipelineSpec.DescriptorSetLayouts = {s_Data.LightingDescriptorSetLayout};

        s_Data.LightingPipeline = MakeRef<VulkanPipeline>(PipelineSpec);

        LightingVertexShader->Destroy();
        LightingFragmentShader->Destroy();
    }
}

void VulkanRenderer::BeginSceneImpl(const PerspectiveCamera& camera)
{
    s_Data.MeshCameraDataBuffer.Projection = camera.GetProjectionMatrix();
    s_Data.MeshCameraDataBuffer.View       = camera.GetViewMatrix();
    s_Data.MeshCameraDataBuffer.Position   = camera.GetPosition();

    memcpy(s_Data.MappedUniformCameraDataBuffers[m_Context.GetSwapchain()->GetCurrentFrameIndex()], &s_Data.MeshCameraDataBuffer,
           sizeof(CameraDataBuffer));

    VulkanRenderer2D::GetStorageData().CameraProjectionMatrix = camera.GetViewProjectionMatrix();
    s_Data.SkyboxPushConstants.TransformMatrix =
        camera.GetProjectionMatrix() *
        glm::mat4(glm::mat3(camera.GetViewMatrix()));  // Removing 4th column of view martix which contains translation;
}

void VulkanRenderer::BeginImpl()
{
    Renderer::GetStats().DrawCalls = 0;

    if (s_Data.bFramebuffersNeedResize)
    {
        m_Context.GetDevice()->WaitDeviceOnFinish();
        s_Data.GeometryFramebuffer->Resize(s_Data.NewFramebufferSize.x, s_Data.NewFramebufferSize.y);
        s_Data.SetupFramebuffer->Resize(s_Data.NewFramebufferSize.x, s_Data.NewFramebufferSize.y);

        /*for (auto& geometry : s_Data.SortedGeometry)
        {
            geometry.Material->Invalidate();
        }*/

        // Implicit image transition.
        VkCommandBuffer commandBuffer =
            Utility::BeginSingleTimeCommands(m_Context.GetGraphicsCommandPool()->Get(), m_Context.GetDevice()->GetLogicalDevice());

        s_Data.SetupFramebuffer->BeginRenderPass(commandBuffer);
        s_Data.SetupFramebuffer->EndRenderPass(commandBuffer);

        Utility::EndSingleTimeCommands(commandBuffer, m_Context.GetGraphicsCommandPool()->Get(), m_Context.GetDevice()->GetGraphicsQueue(),
                                       m_Context.GetDevice()->GetLogicalDevice());

        for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
        {
            std::vector<VkWriteDescriptorSet> writes;

            for (uint32_t k = 0; k < s_Data.GeometryFramebuffer->GetAttachments().size() - 1; ++k)
            {
                Ref<VulkanImage> vulkanImage =
                    static_pointer_cast<VulkanImage>(s_Data.GeometryFramebuffer->GetAttachments()[k].Attachments[i]);
                auto textureWriteSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, k,
                                                                      s_Data.LightingSet.Handle, 1, &vulkanImage->GetDescriptorInfo());
                writes.push_back(textureWriteSet);
            }

            VkDescriptorBufferInfo LightingModelBufferInfo = {};
            LightingModelBufferInfo.buffer                 = s_Data.UniformPhongModelBuffers[i].Buffer;
            LightingModelBufferInfo.range                  = sizeof(LightingModelBuffer);
            LightingModelBufferInfo.offset                 = 0;

            // PhongModel
            auto PhongModelBufferWriteSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3, s_Data.LightingSet.Handle,
                                                                           1, &LightingModelBufferInfo);

            writes.push_back(PhongModelBufferWriteSet);

            vkUpdateDescriptorSets(m_Context.GetDevice()->GetLogicalDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0,
                                   nullptr);
        }

        s_Data.bFramebuffersNeedResize = false;
    }

    s_Data.CurrentCommandBuffer = &m_Context.GetGraphicsCommandPool()->GetCommandBuffer(m_Context.GetSwapchain()->GetCurrentFrameIndex());
    GNT_ASSERT(s_Data.CurrentCommandBuffer, "Failed to retrieve command buffer!");

    // Clear geometry buffer contents
    s_Data.CurrentCommandBuffer->BeginDebugLabel("SetupPass", glm::vec4(0.3f, 0.56f, 0.841f, 1.0f));
    s_Data.SetupFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());

    s_Data.SetupFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());
    s_Data.CurrentCommandBuffer->EndDebugLabel();

    {
        s_Data.MeshLightingModelBuffer.Gamma = s_RendererSettings.Gamma;
        // Reset all the light sources to default values in case we deleted them, because in the next frame they will be added to buffer
        // if they weren't removed.
        s_Data.MeshLightingModelBuffer.DirLight = DirectionalLight();

        for (uint32_t i = 0; i < s_Data.CurrentPointLightIndex; ++i)
            s_Data.MeshLightingModelBuffer.PointLights[i] = PointLight();

        memcpy(s_Data.MappedUniformPhongModelBuffers[m_Context.GetSwapchain()->GetCurrentFrameIndex()], &s_Data.MeshLightingModelBuffer,
               sizeof(LightingModelBuffer));
    }
    s_Data.CurrentPointLightIndex = 0;

    s_Data.SortedGeometry.clear();
}

void VulkanRenderer::SubmitMeshImpl(const Ref<Mesh>& mesh, const glm::mat4& transform)
{
    for (uint32_t i = 0; i < mesh->GetSubmeshCount(); ++i)
    {
        const auto vulkanMaterial = static_pointer_cast<Gauntlet::VulkanMaterial>(mesh->GetMaterial(i));
        GNT_ASSERT(vulkanMaterial, "Failed to retrieve mesh material!");

        Ref<Gauntlet::VulkanVertexBuffer> vulkanVertexBuffer =
            std::static_pointer_cast<Gauntlet::VulkanVertexBuffer>(mesh->GetVertexBuffers()[i]);

        Ref<Gauntlet::VulkanIndexBuffer> vulkanIndexBuffer =
            std::static_pointer_cast<Gauntlet::VulkanIndexBuffer>(mesh->GetIndexBuffers()[i]);

        s_Data.SortedGeometry.emplace_back(vulkanMaterial, vulkanVertexBuffer, vulkanIndexBuffer, transform);
    }
}

void VulkanRenderer::AddPointLightImpl(const glm::vec3& position, const glm::vec3& color, const glm::vec3& AmbientSpecularShininess,
                                       const glm::vec3& CLQ)
{
    if (s_Data.CurrentPointLightIndex >= s_MAX_POINT_LIGHTS) return;

    s_Data.MeshLightingModelBuffer.PointLights[s_Data.CurrentPointLightIndex].Position = glm::vec4(position, 0.0f);
    s_Data.MeshLightingModelBuffer.PointLights[s_Data.CurrentPointLightIndex].Color    = glm::vec4(color, 0.0f);
    s_Data.MeshLightingModelBuffer.PointLights[s_Data.CurrentPointLightIndex].AmbientSpecularShininess =
        glm::vec4(AmbientSpecularShininess, 0.0f);
    s_Data.MeshLightingModelBuffer.PointLights[s_Data.CurrentPointLightIndex].CLQ = glm::vec4(CLQ, 0.0f);

    ++s_Data.CurrentPointLightIndex;
}

void VulkanRenderer::AddDirectionalLightImpl(const glm::vec3& color, const glm::vec3& direction, const glm::vec3& AmbientSpecularShininess)
{
    s_Data.MeshLightingModelBuffer.Gamma = s_RendererSettings.Gamma;

    s_Data.MeshLightingModelBuffer.DirLight.Color                    = glm::vec4(color, 0.0f);
    s_Data.MeshLightingModelBuffer.DirLight.Direction                = glm::vec4(direction, 0.0f);
    s_Data.MeshLightingModelBuffer.DirLight.AmbientSpecularShininess = glm::vec4(AmbientSpecularShininess, 0.0f);
}

void VulkanRenderer::SetupSkybox()
{
    const auto SkyboxPath                = std::string("Resources/Textures/Skybox/Yokohama2/");
    const std::vector<std::string> Faces = {
        SkyboxPath + "right.jpg",   //
        SkyboxPath + "left.jpg",    //
        SkyboxPath + "bottom.jpg",  // Swapped because I flip the viewport's Y axis
        SkyboxPath + "top.jpg",     // Swapped
        SkyboxPath + "front.jpg",   //
        SkyboxPath + "back.jpg"     //
    };
    s_Data.DefaultSkybox = MakeRef<Skybox>(Faces);

    const auto CubeMapTextureBinding =
        Utility::GetDescriptorSetLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

    auto SkyboxDescriptorSetLayoutCreateInfo = Utility::GetDescriptorSetLayoutCreateInfo(1, &CubeMapTextureBinding, 0);
    VK_CHECK(vkCreateDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), &SkyboxDescriptorSetLayoutCreateInfo, nullptr,
                                         &s_Data.SkyboxDescriptorSetLayout),
             "Failed to create skybox descriptor set layout!");

    auto SkyboxVertexShader   = Ref<VulkanShader>(new VulkanShader("Resources/Cached/Shaders/Skybox.vert.spv"));
    auto SkyboxFragmentShader = Ref<VulkanShader>(new VulkanShader("Resources/Cached/Shaders/Skybox.frag.spv"));

    s_Data.SkyboxVertexBufferLayout = {
        {EShaderDataType::Vec3, "InPosition"}  //
    };

    // Base skybox pipeline creation
    PipelineSpecification PipelineSpec = {};
    PipelineSpec.Name                  = "Skybox";
    PipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
    PipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
    PipelineSpec.PrimitiveTopology     = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PipelineSpec.CullMode              = ECullMode::CULL_MODE_BACK;
    PipelineSpec.bDepthTest            = VK_TRUE;
    PipelineSpec.bDepthWrite =
        VK_FALSE;  // I think there's no need to write depth in framebuffer because skybox rendered in the end of all passes
    PipelineSpec.DepthCompareOp = VK_COMPARE_OP_LESS;

    PipelineSpec.TargetFramebuffer = s_Data.GeometryFramebuffer /*m_Context.GetMainRenderPass()*/;
    PipelineSpec.ShaderStages      = {
        {SkyboxVertexShader, EShaderStage::SHADER_STAGE_VERTEX},     //
        {SkyboxFragmentShader, EShaderStage::SHADER_STAGE_FRAGMENT}  //
    };

    PipelineSpec.PushConstantRanges        = {Utility::GetPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(MeshPushConstants))};
    PipelineSpec.ShaderBindingDescriptions = {Utility::GetShaderBindingDescription(0, s_Data.SkyboxVertexBufferLayout.GetStride())};

    std::vector<VkVertexInputAttributeDescription> ShaderAttributeDescriptions;
    for (uint32_t i = 0; i < s_Data.SkyboxVertexBufferLayout.GetElements().size(); ++i)
    {
        const auto& CurrentBufferElement = s_Data.SkyboxVertexBufferLayout.GetElements()[i];
        ShaderAttributeDescriptions.emplace_back(Utility::GetShaderAttributeDescription(
            0, Utility::GauntletShaderDataTypeToVulkan(CurrentBufferElement.Type), i, CurrentBufferElement.Offset));
    }
    PipelineSpec.ShaderAttributeDescriptions = ShaderAttributeDescriptions;

    PipelineSpec.DescriptorSetLayouts = {s_Data.SkyboxDescriptorSetLayout};
    s_Data.SkyboxPipeline             = MakeRef<VulkanPipeline>(PipelineSpec);

    SkyboxVertexShader->Destroy();
    SkyboxFragmentShader->Destroy();

    GNT_ASSERT(m_Context.GetDescriptorAllocator()->Allocate(s_Data.SkyboxDescriptorSet, s_Data.SkyboxDescriptorSetLayout),
               "Failed to allocate descriptor sets!");

    auto CubeMapTexture = std::static_pointer_cast<VulkanTextureCube>(s_Data.DefaultSkybox->GetCubeMapTexture());
    GNT_ASSERT(CubeMapTexture, "Skybox image is not valid!");

    auto CubeMapTextureImageInfo        = CubeMapTexture->GetImage()->GetDescriptorInfo();
    const auto SkyboxWriteDescriptorSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
                                                                         s_Data.SkyboxDescriptorSet.Handle, 1, &CubeMapTextureImageInfo);
    vkUpdateDescriptorSets(m_Context.GetDevice()->GetLogicalDevice(), 1, &SkyboxWriteDescriptorSet, 0, VK_NULL_HANDLE);
}

void VulkanRenderer::DrawSkybox()
{
    const auto& Swapchain = m_Context.GetSwapchain();
    GNT_ASSERT(s_Data.CurrentCommandBuffer, "You can't render skybox before you've acquired valid command buffer!");
    s_Data.CurrentCommandBuffer->BeginDebugLabel("Skybox", glm::vec4(0.0f, 0.0f, 0.25f, 1.0f));

    s_Data.CurrentCommandBuffer->BindPipeline(s_Data.SkyboxPipeline);

    static constexpr uint32_t s_MaxSkyboxSize = 460;
    s_Data.SkyboxPushConstants.TransformMatrix *= glm::scale(glm::mat4(1.0f), glm::vec3(s_MaxSkyboxSize));

    s_Data.CurrentCommandBuffer->BindPushConstants(s_Data.SkyboxPipeline->GetLayout(),
                                                   s_Data.SkyboxPipeline->GetPushConstantsShaderStageFlags(), 0,
                                                   s_Data.SkyboxPipeline->GetPushConstantsSize(), &s_Data.SkyboxPushConstants);

    const auto& CubeMesh = s_Data.DefaultSkybox->GetCubeMesh();
    s_Data.CurrentCommandBuffer->BindDescriptorSets(s_Data.SkyboxPipeline->GetLayout(), 0, 1, &s_Data.SkyboxDescriptorSet.Handle);

    VkDeviceSize Offset = 0;
    s_Data.CurrentCommandBuffer->BindVertexBuffers(
        0, 1, &std::static_pointer_cast<VulkanVertexBuffer>(CubeMesh->GetVertexBuffers()[0])->Get(), &Offset);

    Ref<Gauntlet::VulkanIndexBuffer> VulkanIndexBuffer =
        std::static_pointer_cast<Gauntlet::VulkanIndexBuffer>(CubeMesh->GetIndexBuffers()[0]);
    s_Data.CurrentCommandBuffer->BindIndexBuffer(VulkanIndexBuffer->Get(), 0, VK_INDEX_TYPE_UINT32);
    s_Data.CurrentCommandBuffer->DrawIndexed(static_cast<uint32_t>(VulkanIndexBuffer->GetCount()));

    s_Data.CurrentCommandBuffer->EndDebugLabel();
}

void VulkanRenderer::DestroySkybox()
{
    vkDestroyDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), s_Data.SkyboxDescriptorSetLayout, nullptr);

    m_Context.GetDescriptorAllocator()->ReleaseDescriptorSets(&s_Data.SkyboxDescriptorSet, 1);
    s_Data.SkyboxPipeline->Destroy();
    s_Data.DefaultSkybox->Destroy();
}

void VulkanRenderer::EndSceneImpl()
{
    std::sort(s_Data.SortedGeometry.begin(), s_Data.SortedGeometry.end(),
              [&](const GeometryData& lhs, const GeometryData& rhs)
              {
                  const glm::vec3& cameraPos = s_Data.MeshCameraDataBuffer.Position;
                  const glm::vec3 lhsPos(lhs.Transform[3]);
                  const glm::vec3 rhsPos(rhs.Transform[3]);

                  const float lhsLength = glm::length(cameraPos - lhsPos);
                  const float rhsLength = glm::length(cameraPos - rhsPos);

                  return lhsLength > rhsLength;  // Descending order
              });

    GNT_ASSERT(s_Data.CurrentCommandBuffer, "Failed to retrieve command buffer!");

    // Multithreaded stuff

    // Add a job to the thread's queue for each object to be rendered
    const uint32_t threadCount   = JobSystem::GetThreadCount() > 0 ? JobSystem::GetThreadCount() : 1;
    const uint32_t objectsUint32 = (uint32_t)s_Data.SortedGeometry.size() / threadCount;
    bool bAreThereRemainingObjects{false};
    {
        const float objectsFloat  = static_cast<float>(s_Data.SortedGeometry.size()) / threadCount;
        bAreThereRemainingObjects = objectsFloat > static_cast<float>(objectsUint32);
    }

    // TODO: Think if geometry pass objects and shadow pass object vary or not? Should I calculate objects per thread per pass? If I do
    // render some objects in geometry pass, but I do not render them in shadow pass(some reasons may happen)
    const uint32_t objectsPerThread = bAreThereRemainingObjects ? objectsUint32 + 1 : objectsUint32;

    auto& threads    = JobSystem::GetThreads();
    auto& threadData = m_Context.GetThreadData();

    // ShadowMap
    {
        s_Data.ShadowMapFramebuffer->SetDepthStencilClearColor(GetSettings().RenderShadows ? 1.0f : 0.0f, 0);

        s_Data.CurrentCommandBuffer->BeginDebugLabel("ShadowMapPass", glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
        if (!Renderer::GetSettings().RenderShadows)
        {
            s_Data.ShadowMapFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());
        }
        else
        {
            s_Data.ShadowMapFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get(), VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

            constexpr float cameraWidth     = 30.0f;
            constexpr float zNear           = 1.0f;
            constexpr float zFar            = 128.0f;
            const glm::mat4 lightProjection = glm::ortho(-cameraWidth, cameraWidth, -cameraWidth, cameraWidth, zNear, zFar);

            const glm::mat4 lightView =
                glm::lookAt(glm::vec3(s_Data.MeshLightingModelBuffer.DirLight.Direction), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            s_Data.MeshShadowsBuffer.LightSpaceMatrix = lightProjection * lightView;

            LightPushConstants pushConstants   = {};
            pushConstants.LightSpaceProjection = s_Data.MeshShadowsBuffer.LightSpaceMatrix;
            pushConstants.Model                = glm::mat4(1.0f);

            const auto inheritanceInfo =
                Utility::GetCommandBufferInheritanceInfo(s_Data.ShadowMapFramebuffer->GetRenderPass(), s_Data.ShadowMapFramebuffer->Get());

            uint32_t currentGeometryIndex = 0;
            std::vector<VkCommandBuffer> recordedSecondaryShadowMapCommandBuffers;

            for (uint32_t t = 0; t < threadCount; ++t)
            {
                std::vector<GeometryData> objects;
                for (uint32_t i = 0; i < objectsPerThread; ++i)
                {
                    if (currentGeometryIndex >= s_Data.SortedGeometry.size())
                        break;
                    else
                    {
                        objects.push_back(s_Data.SortedGeometry[currentGeometryIndex]);
                        ++currentGeometryIndex;
                    }
                }

                if (objects.empty()) break;

                auto& currentSecondaryCommandBuffer =
                    threadData[t].SecondaryShadowMapCommandBuffers[m_Context.GetSwapchain()->GetCurrentFrameIndex()];
                recordedSecondaryShadowMapCommandBuffers.push_back(currentSecondaryCommandBuffer.Get());
                threads[t].Submit(
                    [=]
                    {
                        currentSecondaryCommandBuffer.BeginRecording(0, &inheritanceInfo);
                        currentSecondaryCommandBuffer.BindPipeline(s_Data.ShadowMapPipeline);

                        for (auto& object : objects)
                        {
                            currentSecondaryCommandBuffer.BindPushConstants(
                                s_Data.ShadowMapPipeline->GetLayout(), s_Data.ShadowMapPipeline->GetPushConstantsShaderStageFlags(), 0,
                                s_Data.ShadowMapPipeline->GetPushConstantsSize(), &pushConstants);

                            VkDeviceSize Offset = 0;
                            currentSecondaryCommandBuffer.BindVertexBuffers(0, 1, &object.VulkanVertexBuffer->Get(), &Offset);

                            currentSecondaryCommandBuffer.BindIndexBuffer(object.VulkanIndexBuffer->Get(), 0, VK_INDEX_TYPE_UINT32);
                            currentSecondaryCommandBuffer.DrawIndexed(static_cast<uint32_t>(object.VulkanIndexBuffer->GetCount()));
                            ++Renderer::GetStats().DrawCalls;
                        }

                        currentSecondaryCommandBuffer.EndRecording();
                    });
            }

            if (!recordedSecondaryShadowMapCommandBuffers.empty())
            {
                JobSystem::Wait();
                vkCmdExecuteCommands(s_Data.CurrentCommandBuffer->Get(), recordedSecondaryShadowMapCommandBuffers.size(),
                                     recordedSecondaryShadowMapCommandBuffers.data());
            }
        }

        s_Data.ShadowMapFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());
        s_Data.CurrentCommandBuffer->EndDebugLabel();
    }

    // Actual GeometryPass
    {
        s_Data.CurrentCommandBuffer->BeginDebugLabel("GeometryPass", glm::vec4(0.5f, 0.0f, 0.0f, 1.0f));

        if (GetSettings().DisplayShadowMapRenderTarget)
        {
            s_Data.GeometryFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());
            s_Data.CurrentCommandBuffer->BindPipeline(s_Data.DebugShadowMapPipeline);
            s_Data.CurrentCommandBuffer->BindDescriptorSets(s_Data.DebugShadowMapPipeline->GetLayout(), 0, 1,
                                                            &s_Data.DebugShadowMapDescriptorSet.Handle);  // set no layout

            s_Data.CurrentCommandBuffer->Draw(3, 1);
        }
        else
        {
            s_Data.GeometryFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get(), VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

            //// Set shadow buffer(only contains light space mvp mat4)
            // memcpy(s_Data.MappedUniformShadowsBuffers[m_Context.GetSwapchain()->GetCurrentFrameIndex()], &s_Data.MeshShadowsBuffer,
            //        sizeof(ShadowsBuffer));

            // Set lighting stuff
            memcpy(s_Data.MappedUniformPhongModelBuffers[m_Context.GetSwapchain()->GetCurrentFrameIndex()], &s_Data.MeshLightingModelBuffer,
                   sizeof(LightingModelBuffer));

            const auto inheritanceInfo =
                Utility::GetCommandBufferInheritanceInfo(s_Data.GeometryFramebuffer->GetRenderPass(), s_Data.GeometryFramebuffer->Get());

            // TODO: Handle while there's no multithreading.
            uint32_t currentGeometryIndex = 0;
            std::vector<VkCommandBuffer> recordedSecondaryGeometryCommandBuffers;

            for (uint32_t t = 0; t < threadCount; ++t)
            {
                std::vector<GeometryData> objects;
                for (uint32_t i = 0; i < objectsPerThread; ++i)
                {
                    if (currentGeometryIndex >= s_Data.SortedGeometry.size())
                        break;
                    else
                    {
                        objects.push_back(s_Data.SortedGeometry[currentGeometryIndex]);
                        ++currentGeometryIndex;
                    }
                }

                if (objects.empty()) break;

                auto& currentSecondaryCommandBuffer =
                    threadData[t].SecondaryGeometryCommandBuffers[m_Context.GetSwapchain()->GetCurrentFrameIndex()];
                recordedSecondaryGeometryCommandBuffers.push_back(currentSecondaryCommandBuffer.Get());
                threads[t].Submit(
                    [=]
                    {
                        currentSecondaryCommandBuffer.BeginRecording(0, &inheritanceInfo);
                        // currentSecondaryCommandBuffer.SetPipelinePolygonMode(s_Data.GeometryPipeline, GetSettings().ShowWireframes
                        //                                                                                   ?
                        //                                                                                   EPolygonMode::POLYGON_MODE_LINE
                        //                                                                                   :
                        //                                                                                   EPolygonMode::POLYGON_MODE_FILL);
                        //  currentSecondaryCommandBuffer.BindPipeline(s_Data.GeometryPipeline);
                        currentSecondaryCommandBuffer.BindPipeline(s_Data.DefferedPipeline);

                        for (auto& object : objects)
                        {
                            MeshPushConstants PushConstants = {};
                            PushConstants.TransformMatrix   = object.Transform;

                            /*currentSecondaryCommandBuffer.BindPushConstants(s_Data.GeometryPipeline->GetLayout(),
                                                                            s_Data.GeometryPipeline->GetPushConstantsShaderStageFlags(),
                               0, s_Data.GeometryPipeline->GetPushConstantsSize(), &PushConstants);*/

                            currentSecondaryCommandBuffer.BindPushConstants(
                                s_Data.DefferedPipeline->GetLayout(), s_Data.DefferedPipeline->GetPushConstantsShaderStageFlags(), 0,
                                s_Data.DefferedPipeline->GetPushConstantsSize(), &PushConstants);

                            auto MaterialDescriptorSet = object.Material->GetDescriptorSet();
                            /*          currentSecondaryCommandBuffer.BindDescriptorSets(s_Data.GeometryPipeline->GetLayout(), 0, 1,
                                                                                       &MaterialDescriptorSet);*/

                            currentSecondaryCommandBuffer.BindDescriptorSets(s_Data.DefferedPipeline->GetLayout(), 0, 1,
                                                                             &MaterialDescriptorSet);

                            VkDeviceSize Offset = 0;
                            currentSecondaryCommandBuffer.BindVertexBuffers(0, 1, &object.VulkanVertexBuffer->Get(), &Offset);

                            currentSecondaryCommandBuffer.BindIndexBuffer(object.VulkanIndexBuffer->Get(), 0, VK_INDEX_TYPE_UINT32);
                            currentSecondaryCommandBuffer.DrawIndexed(static_cast<uint32_t>(object.VulkanIndexBuffer->GetCount()));
                            ++Renderer::GetStats().DrawCalls;
                        }

                        currentSecondaryCommandBuffer.EndRecording();
                    });
            }

            if (!recordedSecondaryGeometryCommandBuffers.empty())
            {
                JobSystem::Wait();
                vkCmdExecuteCommands(s_Data.CurrentCommandBuffer->Get(), recordedSecondaryGeometryCommandBuffers.size(),
                                     recordedSecondaryGeometryCommandBuffers.data());
            }
        }

        s_Data.GeometryFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());
        s_Data.CurrentCommandBuffer->EndDebugLabel();
    }

    // Render skybox as last geometry to prevent depth testing as mush as possible
    s_Data.CurrentCommandBuffer->BeginDebugLabel("Skybox", glm::vec4(0.5f, 0.0f, 0.0f, 1.0f));
    s_Data.GeometryFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());

    DrawSkybox();

    s_Data.GeometryFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());
    s_Data.CurrentCommandBuffer->EndDebugLabel();
}

void VulkanRenderer::FlushImpl()
{
    s_Data.CurrentCommandBuffer->BeginDebugLabel("LightingPass", glm::vec4(0.7f, 0.7f, 0.0f, 1.0f));
    s_Data.LightingFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());

    s_Data.CurrentCommandBuffer->BindPipeline(s_Data.LightingPipeline);

    MeshPushConstants pushConstants = {};
    pushConstants.Color             = glm::vec4(s_Data.MeshCameraDataBuffer.Position, 0.0f);

    s_Data.CurrentCommandBuffer->BindPushConstants(s_Data.LightingPipeline->GetLayout(),
                                                   s_Data.LightingPipeline->GetPushConstantsShaderStageFlags(), 0,
                                                   s_Data.LightingPipeline->GetPushConstantsSize(), &pushConstants);
    s_Data.CurrentCommandBuffer->BindDescriptorSets(s_Data.LightingPipeline->GetLayout(), 0, 1, &s_Data.LightingSet.Handle);
    s_Data.CurrentCommandBuffer->Draw(6);

    s_Data.LightingFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());
    s_Data.CurrentCommandBuffer->EndDebugLabel();
}

void VulkanRenderer::ResizeFramebuffersImpl(uint32_t width, uint32_t height)
{
    // Resize all framebuffers

    s_Data.bFramebuffersNeedResize = true;
    s_Data.NewFramebufferSize      = {width, height};
}

const Ref<Image>& VulkanRenderer::GetFinalImageImpl()
{
    return s_Data.LightingFramebuffer->GetAttachments()[0].Attachments[m_Context.GetSwapchain()->GetCurrentFrameIndex()];
}

std::vector<RendererOutput> VulkanRenderer::GetRendererOutputImpl()
{
    std::vector<RendererOutput> rendererOutput;

    const uint32_t currentFrame = m_Context.GetSwapchain()->GetCurrentFrameIndex();

    rendererOutput.emplace_back(s_Data.ShadowMapFramebuffer->GetDepthAttachment(), "ShadowMap");
    rendererOutput.emplace_back(s_Data.GeometryFramebuffer->GetAttachments()[0].Attachments[currentFrame], "Position");
    rendererOutput.emplace_back(s_Data.GeometryFramebuffer->GetAttachments()[1].Attachments[currentFrame], "Normal");
    rendererOutput.emplace_back(s_Data.GeometryFramebuffer->GetAttachments()[2].Attachments[currentFrame], "Albedo");
    rendererOutput.emplace_back(s_Data.GeometryFramebuffer->GetAttachments()[3].Attachments[currentFrame], "Depth");

    return rendererOutput;
}

void VulkanRenderer::Destroy()
{
    m_Context.GetDevice()->WaitDeviceOnFinish();

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        m_Context.GetAllocator()->Unmap(s_Data.UniformCameraDataBuffers[i].Allocation);
        BufferUtils::DestroyBuffer(s_Data.UniformCameraDataBuffers[i]);

        m_Context.GetAllocator()->Unmap(s_Data.UniformPhongModelBuffers[i].Allocation);
        BufferUtils::DestroyBuffer(s_Data.UniformPhongModelBuffers[i]);

        m_Context.GetAllocator()->Unmap(s_Data.UniformShadowsBuffers[i].Allocation);
        BufferUtils::DestroyBuffer(s_Data.UniformShadowsBuffers[i]);
    }

    s_Data.ShadowMapFramebuffer->Destroy();
    s_Data.GeometryFramebuffer->Destroy();
    s_Data.SetupFramebuffer->Destroy();
    s_Data.LightingFramebuffer->Destroy();

    s_Data.GeometryPipeline->Destroy();
    s_Data.ShadowMapPipeline->Destroy();
    s_Data.DebugShadowMapPipeline->Destroy();
    s_Data.DefferedPipeline->Destroy();
    s_Data.LightingPipeline->Destroy();

    vkDestroyDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), s_Data.DebugShadowMapDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), s_Data.MeshDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), s_Data.ImageDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), s_Data.DefferedDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), s_Data.LightingDescriptorSetLayout, nullptr);

    s_Data.MeshWhiteTexture->Destroy();
    s_Data.CurrentCommandBuffer = nullptr;

    DestroySkybox();
}

}  // namespace Gauntlet