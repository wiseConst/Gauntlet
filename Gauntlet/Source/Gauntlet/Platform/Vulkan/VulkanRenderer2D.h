#pragma once

#include "Gauntlet/Renderer/Renderer2D.h"
#include "VulkanBuffer.h"

namespace Gauntlet
{

class Texture2D;
class VulkanTexture2D;
class VulkanContext;
class VulkanShader;
class VulkanPipeline;

class VulkanRenderer2D final : public Renderer2D
{
  private:
    // The way it works: Allocate one big staging buffer and copy it's data to vertex buffer, that's it.
    struct VulkanStagingStorage
    {
        struct StagingBuffer
        {
            AllocatedBuffer Buffer;
            size_t Capacity = 0;
            size_t Size     = 0;
        };

        std::vector<Scoped<StagingBuffer>> StagingBuffers;
        uint32_t CurrentStagingBufferIndex = 0;
    };

    struct VulkanRenderer2DStorage
    {
        static constexpr uint32_t MaxQuads          = 2500;
        static constexpr uint32_t MaxVertices       = MaxQuads * 4;
        static constexpr uint32_t MaxIndices        = MaxQuads * 6;
        static constexpr uint32_t MaxTextureSlots   = 32;
        static constexpr glm::vec2 TextureCoords[4] = {glm::vec2(0.0f, 0.0f),  //
                                                       glm::vec2(1.0f, 0.0f),  //
                                                       glm::vec2(1.0f, 1.0f),  //
                                                       glm::vec2(0.0f, 1.0f)};

        static constexpr glm::vec4 QuadVertexPositions[4] = {{-0.5f, -0.5f, 0.0f, 1.0f},  //
                                                             {0.5f, -0.5f, 0.0f, 1.0f},   //
                                                             {0.5f, 0.5f, 0.0f, 1.0f},    //
                                                             {-0.5f, 0.5f, 0.0f, 1.0f}};

        static constexpr size_t DefaultVertexBufferSize = MaxVertices * sizeof(QuadVertex);

        VulkanStagingStorage StagingStorage;
        std::vector<Ref<VulkanVertexBuffer>> QuadVertexBuffers;
        uint32_t CurrentQuadVertexBufferIndex = 0;

        BufferLayout VertexBufferLayout;
        QuadVertex* QuadVertexBufferBase = nullptr;
        QuadVertex* QuadVertexBufferPtr  = nullptr;

        Ref<VulkanIndexBuffer> QuadIndexBuffer;
        uint32_t QuadIndexCount = 0;

        Ref<VulkanPipeline> QuadPipeline;
        Ref<VulkanShader> VertexShader;
        Ref<VulkanShader> FragmentShader;

        VkDescriptorSetLayout QuadDescriptorSetLayout;
        std::vector<VkDescriptorSet> QuadDescriptorSets;
        uint32_t CurrentDescriptorSetIndex = 0;

        std::array<Ref<VulkanTexture2D>, MaxTextureSlots> TextureSlots;
        Ref<VulkanTexture2D> QuadWhiteTexture;  // Texture Slot 0 = white tex
        uint32_t CurrentTextureSlotIndex = 1;   // 0 slot already tied with white texture

        glm::mat4 CameraProjectionMatrix = glm::mat4(1.0f);
        MeshPushConstants PushConstants;
    };

  public:
    VulkanRenderer2D();
    ~VulkanRenderer2D() = default;

    void Create() final override;
    void Destroy() final override;

    void BeginImpl() final override;
    void FlushImpl() final override;

    void DrawQuadImpl(const glm::vec3& InPosition, const glm::vec2& InSize, const glm::vec4& InColor) final override;

    void DrawQuadImpl(const glm::vec3& InPosition, const glm::vec2& InSize, const glm::vec3& InRotation, const Ref<Texture2D>& InTexture,
                      const glm::vec4& InColor) final override;

    void DrawQuadImpl(const glm::mat4& InTransform, const glm::vec4& InColor) final override;

    void DrawRotatedQuadImpl(const glm::vec3& InPosition, const glm::vec2& InSize, const glm::vec3& InRotation,
                             const glm::vec4& InColor) final override;

    void DrawTexturedQuadImpl(const glm::vec3& InPosition, const glm::vec2& InSize, const Ref<Texture2D>& InTexture,
                              const glm::vec4& InBlendColor) final override;

    static VulkanRenderer2DStorage& GetStorageData() { return s_Data2D; }

  private:
    VulkanContext& m_Context;
    inline static VulkanRenderer2DStorage s_Data2D;

    void FlushAndReset();
    void PreallocateRenderStorage();
};

}  // namespace Gauntlet