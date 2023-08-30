#include "GauntletPCH.h"
#include "VulkanRenderer.h"

#include "Gauntlet/Renderer/Camera/PerspectiveCamera.h"

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
        s_Data.MeshWhiteTexture.reset(new VulkanTexture2D(&WhiteTexutreData, sizeof(WhiteTexutreData), 1, 1));
    }

    // TODO: Add filtering(nearest) && wrapping
    {
        FramebufferSpecification shadowMapFramebufferSpec = {};

        FramebufferAttachmentSpecification shadowMapAttachment = {};
        shadowMapAttachment.ClearColor                         = glm::vec4(1.0f, glm::vec3(0.0f));
        shadowMapAttachment.Format                             = EImageFormat::DEPTH32F;
        shadowMapAttachment.LoadOp                             = ELoadOp::CLEAR;
        shadowMapAttachment.StoreOp                            = EStoreOp::STORE;

        shadowMapFramebufferSpec.Attachments = {shadowMapAttachment};
        shadowMapFramebufferSpec.Name        = "ShadowMap";
        shadowMapFramebufferSpec.Width       = 2048;
        shadowMapFramebufferSpec.Height      = 2048;

        s_Data.ShadowMapFramebuffer.reset(new VulkanFramebuffer(shadowMapFramebufferSpec));
    }

    {
        FramebufferSpecification geometryFramebufferSpec = {};

        FramebufferAttachmentSpecification attachment = {};
        attachment.ClearColor                         = glm::vec4(glm::vec3(0.1f), 1.0f);
        attachment.Format                             = EImageFormat::RGBA;
        attachment.LoadOp                             = ELoadOp::LOAD;
        attachment.StoreOp                            = EStoreOp::STORE;
        geometryFramebufferSpec.Attachments.push_back(attachment);

        attachment.ClearColor = glm::vec4(1.0f, glm::vec3(0.0f));
        attachment.Format     = EImageFormat::DEPTH32F;
        geometryFramebufferSpec.Attachments.push_back(attachment);

        geometryFramebufferSpec.Name = "Geometry";

        s_Data.GeometryFramebuffer.reset(new VulkanFramebuffer(geometryFramebufferSpec));
    }

    {
        FramebufferSpecification setupFramebufferSpec = {};
        for (auto& colorAttachment : s_Data.GeometryFramebuffer->GetColorAttachments())
        {
            setupFramebufferSpec.AliveAttachments.push_back(std::static_pointer_cast<Image>(colorAttachment));
        }
        setupFramebufferSpec.AliveAttachments.push_back(std::static_pointer_cast<Image>(s_Data.GeometryFramebuffer->GetDepthAttachment()));
        setupFramebufferSpec.Name = "Setup";

        s_Data.SetupFramebuffer.reset(new VulkanFramebuffer(setupFramebufferSpec));
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

    {
        PipelineSpecification PipelineSpec = {};
        PipelineSpec.Name                  = "Mesh3D";
        PipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        PipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        PipelineSpec.PrimitiveTopology     = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        PipelineSpec.CullMode              = ECullMode::CULL_MODE_NONE;
        PipelineSpec.bDepthTest            = VK_TRUE;
        PipelineSpec.bDepthWrite           = VK_TRUE;
        PipelineSpec.DepthCompareOp        = VK_COMPARE_OP_LESS;
        PipelineSpec.TargetFramebuffer     = s_Data.GeometryFramebuffer;

        auto GeometryVertexShader   = Ref<VulkanShader>(new VulkanShader("Resources/Shaders/Mesh.vert.spv"));
        auto GeometryFragmentShader = Ref<VulkanShader>(new VulkanShader("Resources/Shaders/Mesh.frag.spv"));

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
        s_Data.GeometryPipeline.reset(new VulkanPipeline(PipelineSpec));

        PipelineSpec.Name        = "Mesh3D-Wireframe";
        PipelineSpec.PolygonMode = EPolygonMode::POLYGON_MODE_LINE;
        s_Data.MeshWireframePipeline.reset(new VulkanPipeline(PipelineSpec));

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

    {
        auto DepthVertexShader   = Ref<VulkanShader>(new VulkanShader("Resources/Shaders/Depth.vert.spv"));
        auto DepthFragmentShader = Ref<VulkanShader>(new VulkanShader("Resources/Shaders/Depth.frag.spv"));

        PipelineSpecification PipelineSpec = {};
        PipelineSpec.Name                  = "ShadowMap";
        PipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        PipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        PipelineSpec.CullMode              = ECullMode::CULL_MODE_BACK;
        PipelineSpec.bDepthWrite           = VK_TRUE;
        PipelineSpec.bDepthTest            = VK_TRUE;
        PipelineSpec.DepthCompareOp        = VK_COMPARE_OP_LESS;
        PipelineSpec.TargetFramebuffer     = s_Data.ShadowMapFramebuffer;
        PipelineSpec.PushConstantRanges    = {Utility::GetPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(LightPushConstants))};
        PipelineSpec.ShaderStages          = {{DepthVertexShader, EShaderStage::SHADER_STAGE_VERTEX},
                                     {DepthFragmentShader, EShaderStage::SHADER_STAGE_FRAGMENT}};

        PipelineSpec.ShaderBindingDescriptions = {Utility::GetShaderBindingDescription(0, s_Data.MeshVertexBufferLayout.GetStride())};
        PipelineSpec.ShaderAttributeDescriptions.emplace_back(0, 0, Utility::GauntletShaderDataTypeToVulkan(EShaderDataType::Vec3), 0);

        s_Data.ShadowMapPipeline.reset(new VulkanPipeline(PipelineSpec));

        DepthVertexShader->Destroy();
        DepthFragmentShader->Destroy();
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
        // s_Data.ShadowMapFramebuffer->Resize(s_Data.NewFramebufferSize.x, s_Data.NewFramebufferSize.y);
        s_Data.GeometryFramebuffer->Resize(s_Data.NewFramebufferSize.x, s_Data.NewFramebufferSize.y);
        s_Data.SetupFramebuffer->Resize(s_Data.NewFramebufferSize.x, s_Data.NewFramebufferSize.y);

        // for (auto& geometry : s_Data.SortedGeometry)
        //{
        //     geometry.Material->Invalidate();
        // }

        s_Data.bFramebuffersNeedResize = false;
    }

    s_Data.CurrentCommandBuffer = &m_Context.GetGraphicsCommandPool()->GetCommandBuffer(m_Context.GetSwapchain()->GetCurrentFrameIndex());
    GNT_ASSERT(s_Data.CurrentCommandBuffer, "Failed to retrieve command buffer!");

    // Clear geometry buffer contents
    s_Data.CurrentCommandBuffer->BeginDebugLabel("ClearPass", glm::vec4(0.3f, 0.56f, 0.841f, 1.0f));
    s_Data.SetupFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());

    s_Data.SetupFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());
    s_Data.CurrentCommandBuffer->EndDebugLabel();

    {
        s_Data.MeshLightingModelBuffer.Gamma = s_RendererSettings.Gamma;
        // Reset all the light sources to default values in case we deleted them, because in the next frame they will be added to buffer if
        // they weren't removed.
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

        s_Data.SortedGeometry.emplace_back(mesh->GetSubmeshName(i), vulkanMaterial, vulkanVertexBuffer, vulkanIndexBuffer, transform);
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

    memcpy(s_Data.MappedUniformPhongModelBuffers[m_Context.GetSwapchain()->GetCurrentFrameIndex()], &s_Data.MeshLightingModelBuffer,
           sizeof(LightingModelBuffer));
    ++s_Data.CurrentPointLightIndex;
}

void VulkanRenderer::AddDirectionalLightImpl(const glm::vec3& color, const glm::vec3& direction, const glm::vec3& AmbientSpecularShininess)
{
    s_Data.MeshLightingModelBuffer.Gamma = s_RendererSettings.Gamma;

    s_Data.MeshLightingModelBuffer.DirLight.Color                    = glm::vec4(color, 0.0f);
    s_Data.MeshLightingModelBuffer.DirLight.Direction                = glm::vec4(direction, 0.0f);
    s_Data.MeshLightingModelBuffer.DirLight.AmbientSpecularShininess = glm::vec4(AmbientSpecularShininess, 0.0f);

    memcpy(s_Data.MappedUniformPhongModelBuffers[m_Context.GetSwapchain()->GetCurrentFrameIndex()], &s_Data.MeshLightingModelBuffer,
           sizeof(LightingModelBuffer));
}

void VulkanRenderer::SetupSkybox()
{
    const auto SkyboxPath                = std::string("Resources/Textures/Skybox/SanFrancisco3/");
    const std::vector<std::string> Faces = {
        SkyboxPath + "right.jpg",   //
        SkyboxPath + "left.jpg",    //
        SkyboxPath + "bottom.jpg",  // Swapped because I flip the viewport's Y axis
        SkyboxPath + "top.jpg",     // Swapped
        SkyboxPath + "front.jpg",   //
        SkyboxPath + "back.jpg"     //
    };
    s_Data.DefaultSkybox.reset(new Skybox(Faces));

    const auto CubeMapTextureBinding =
        Utility::GetDescriptorSetLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

    auto SkyboxDescriptorSetLayoutCreateInfo = Utility::GetDescriptorSetLayoutCreateInfo(1, &CubeMapTextureBinding, 0);
    VK_CHECK(vkCreateDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), &SkyboxDescriptorSetLayoutCreateInfo, nullptr,
                                         &s_Data.SkyboxDescriptorSetLayout),
             "Failed to create skybox descriptor set layout!");

    auto SkyboxVertexShader   = Ref<VulkanShader>(new VulkanShader("Resources/Shaders/Skybox.vert.spv"));
    auto SkyboxFragmentShader = Ref<VulkanShader>(new VulkanShader("Resources/Shaders/Skybox.frag.spv"));

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
    s_Data.SkyboxPipeline.reset(new VulkanPipeline(PipelineSpec));

    SkyboxVertexShader->Destroy();
    SkyboxFragmentShader->Destroy();

    GNT_ASSERT(m_Context.GetDescriptorAllocator()->Allocate(s_Data.SkyboxDescriptorSet, s_Data.SkyboxDescriptorSetLayout),
               "Failed to allocate descriptor sets!");

    VkWriteDescriptorSet SkyboxWriteDescriptorSet = {};
    auto CubeMapTexture                           = std::static_pointer_cast<VulkanTextureCube>(s_Data.DefaultSkybox->GetCubeMapTexture());
    GNT_ASSERT(CubeMapTexture, "Skybox image is not valid!");

    auto CubeMapTextureImageInfo = CubeMapTexture->GetImage()->GetDescriptorInfo();
    SkyboxWriteDescriptorSet     = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
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
                  const glm::vec3 lhsPos(lhs.Transform[3][0], lhs.Transform[3][1], lhs.Transform[3][2]);
                  const glm::vec3 rhsPos(rhs.Transform[3][0], rhs.Transform[3][1], rhs.Transform[3][2]);

                  const float lhsLength = glm::length(cameraPos - lhsPos);
                  const float rhsLength = glm::length(cameraPos - rhsPos);

                  return lhsLength > rhsLength;  // Descending order
              });

    GNT_ASSERT(s_Data.CurrentCommandBuffer, "Failed to retrieve command buffer!");

    // ShadowMap
    s_Data.ShadowMapFramebuffer->SetDepthStencilClearColor(GetSettings().RenderShadows ? 1.0f : 0.0f, 0);

    s_Data.CurrentCommandBuffer->BeginDebugLabel("ShadowMapPass", glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
    s_Data.ShadowMapFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());

    if (GetSettings().RenderShadows)
    {
        const float cameraWidth = 10.0f;
        const float viewportAR  = s_Data.NewFramebufferSize.x / (float)s_Data.NewFramebufferSize.y;
        /* glm::mat4 lightProjection =
             glm::ortho(-cameraWidth * viewportAR, cameraWidth * viewportAR, -cameraWidth, cameraWidth, 1.0f, 96.0f);

         glm::mat4 lightView =  glm::lookAt(glm::vec3(s_Data.MeshLightingModelBuffer.DirLight.Direction), glm::vec3(0.0f, 0.0f, 0.0f),
         glm::vec3(0.0f, 1.0f, 0.0f));*/

        glm::mat4 lightProjection = glm::perspective(45.0f, 1.0f, 1.0f, 96.0f);

        glm::mat4 lightView = glm::lookAt(glm::vec3(s_Data.MeshLightingModelBuffer.PointLights[0].Position), glm::vec3(0.0f, 0.0f, 0.0f),
                                          glm::vec3(0.0f, 1.0f, 0.0f));
        s_Data.MeshShadowsBuffer.LightSpaceMatrix = lightProjection * lightView;

        LightPushConstants pushConstants   = {};
        pushConstants.LightSpaceProjection = s_Data.MeshShadowsBuffer.LightSpaceMatrix;
        pushConstants.Model                = glm::mat4(1.0f);

        s_Data.CurrentCommandBuffer->BindPipeline(s_Data.ShadowMapPipeline);

        for (const auto& geometry : s_Data.SortedGeometry)
        {
            s_Data.CurrentCommandBuffer->BindPushConstants(s_Data.ShadowMapPipeline->GetLayout(),
                                                           s_Data.ShadowMapPipeline->GetPushConstantsShaderStageFlags(), 0,
                                                           s_Data.ShadowMapPipeline->GetPushConstantsSize(), &pushConstants);

            VkDeviceSize Offset = 0;
            s_Data.CurrentCommandBuffer->BindVertexBuffers(0, 1, &geometry.VulkanVertexBuffer->Get(), &Offset);

            s_Data.CurrentCommandBuffer->BindIndexBuffer(geometry.VulkanIndexBuffer->Get(), 0, VK_INDEX_TYPE_UINT32);
            s_Data.CurrentCommandBuffer->DrawIndexed(static_cast<uint32_t>(geometry.VulkanIndexBuffer->GetCount()));
            ++Renderer::GetStats().DrawCalls;
        }
    }

    s_Data.ShadowMapFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());
    s_Data.CurrentCommandBuffer->EndDebugLabel();

    // Actual GeometryPass
    s_Data.CurrentCommandBuffer->BeginDebugLabel("GeometryPass", glm::vec4(0.5f, 0.0f, 0.0f, 1.0f));
    s_Data.GeometryFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());

    memcpy(s_Data.MappedUniformShadowsBuffers[m_Context.GetSwapchain()->GetCurrentFrameIndex()], &s_Data.MeshShadowsBuffer,
           sizeof(ShadowsBuffer));

    s_Data.CurrentCommandBuffer->BindPipeline(GetSettings().ShowWireframes ? s_Data.MeshWireframePipeline : s_Data.GeometryPipeline);

    for (const auto& geometry : s_Data.SortedGeometry)
    {
        MeshPushConstants PushConstants = {};
        PushConstants.TransformMatrix   = geometry.Transform;

        s_Data.CurrentCommandBuffer->BindPushConstants(s_Data.GeometryPipeline->GetLayout(),
                                                       s_Data.GeometryPipeline->GetPushConstantsShaderStageFlags(), 0,
                                                       s_Data.GeometryPipeline->GetPushConstantsSize(), &PushConstants);

        auto MaterialDescriptorSet = geometry.Material->GetDescriptorSet();
        s_Data.CurrentCommandBuffer->BindDescriptorSets(s_Data.GeometryPipeline->GetLayout(), 0, 1, &MaterialDescriptorSet);

        VkDeviceSize Offset = 0;
        s_Data.CurrentCommandBuffer->BindVertexBuffers(0, 1, &geometry.VulkanVertexBuffer->Get(), &Offset);

        s_Data.CurrentCommandBuffer->BindIndexBuffer(geometry.VulkanIndexBuffer->Get(), 0, VK_INDEX_TYPE_UINT32);
        s_Data.CurrentCommandBuffer->DrawIndexed(static_cast<uint32_t>(geometry.VulkanIndexBuffer->GetCount()));
        ++Renderer::GetStats().DrawCalls;
    }

    DrawSkybox();

    s_Data.GeometryFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());
    s_Data.CurrentCommandBuffer->EndDebugLabel();
}

void VulkanRenderer::FlushImpl()
{
    // TODO: Refactor
}

void VulkanRenderer::ResizeFramebuffersImpl(uint32_t width, uint32_t height)
{
    // Resize all framebuffers
    s_Data.bFramebuffersNeedResize = true;
    s_Data.NewFramebufferSize      = {width, height};
}

const Ref<Image> VulkanRenderer::GetFinalImageImpl()
{
    return s_Data.GeometryFramebuffer->GetColorAttachments()[m_Context.GetSwapchain()->GetCurrentImageIndex()];
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

    s_Data.MeshWireframePipeline->Destroy();
    s_Data.GeometryPipeline->Destroy();
    s_Data.ShadowMapPipeline->Destroy();

    vkDestroyDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), s_Data.MeshDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), s_Data.ImageDescriptorSetLayout, nullptr);

    s_Data.MeshWhiteTexture->Destroy();
    s_Data.CurrentCommandBuffer = nullptr;

    DestroySkybox();
}

}  // namespace Gauntlet