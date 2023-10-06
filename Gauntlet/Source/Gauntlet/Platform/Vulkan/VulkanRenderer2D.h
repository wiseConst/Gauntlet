#pragma once

#include "Gauntlet/Renderer/Renderer2D.h"
#include "VulkanBuffer.h"
#include "VulkanDescriptors.h"

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
    struct VulkanRenderer2DStorage
    {
        // Compile-time constants
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

        // Quad staging storage
        struct StagingBuffer
        {
            VulkanAllocatedBuffer Buffer;
            size_t Capacity = 0;
        };
        Scoped<StagingBuffer> QuadStagingBuffer;
        std::vector<Ref<VulkanVertexBuffer>> QuadVertexBuffers;
        uint32_t CurrentQuadVertexBufferIndex = 0;

        // Quad2D Base Stuff
        BufferLayout VertexBufferLayout;
        QuadVertex* QuadVertexBufferBase = nullptr;
        QuadVertex* QuadVertexBufferPtr  = nullptr;

        Ref<VulkanPipeline> QuadPipeline;
        Ref<VulkanIndexBuffer> QuadIndexBuffer;
        uint32_t QuadIndexCount = 0;

        // Textures && descriptors
        VkDescriptorSetLayout QuadDescriptorSetLayout;
        std::vector<DescriptorSet> QuadDescriptorSets;
        uint32_t CurrentDescriptorSetIndex = 0;

        std::array<Ref<VulkanTexture2D>, MaxTextureSlots> TextureSlots;
        uint32_t CurrentTextureSlotIndex = 1;  // 0 slot already tied with white texture

        // Uniform related stuff
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

    void DrawQuadImpl(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color) final override;

    void DrawQuadImpl(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation, const Ref<Texture2D>& texture,
                      const glm::vec4& color) final override;

    void DrawQuadImpl(const glm::mat4& transform, const glm::vec4& color) final override;

    void DrawRotatedQuadImpl(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation,
                             const glm::vec4& color) final override;

    void DrawTexturedQuadImpl(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture,
                              const glm::vec4& blendColor) final override;

    void DrawTexturedQuadImpl(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation,
                              const Ref<Texture2D>& textureAtlas, const glm::vec2& spriteCoords,
                              const glm::vec2& spriteSize) final override;

    static VulkanRenderer2DStorage& GetStorageData() { return s_Data2D; }

  private:
    VulkanContext& m_Context;
    inline static VulkanRenderer2DStorage s_Data2D;

    void FlushAndReset();
    void PreallocateRenderStorage();

    // Final helper-only function
    void DrawQuadInternal(const glm::mat4& transform, const glm::vec4& blendColor, const float textureID);
};

}  // namespace Gauntlet