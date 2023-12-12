#include "GauntletPCH.h"
#include "Renderer2D.h"

#include "RendererAPI.h"
#include "CoreRendererTypes.h"
#include "Texture.h"
#include "Renderer.h"
#include "Buffer.h"
#include "GraphicsContext.h"
#include "Pipeline.h"
#include "Framebuffer.h"

#include "Gauntlet/Platform/Vulkan/VulkanRenderer.h"

namespace Gauntlet
{
Renderer2D::RendererStorage2D* Renderer2D::s_RendererStorage2D = new Renderer2D::RendererStorage2D();

void Renderer2D::Init()
{
    // Index Buffer creation
    {
        uint32_t* QuadIndices = new uint32_t[s_RendererStorage2D->MaxIndices];

        // Clockwise order
        uint32_t Offset = 0;
        for (uint32_t i = 0; i < s_RendererStorage2D->MaxIndices; i += 6)
        {
            QuadIndices[i + 0] = Offset + 0;
            QuadIndices[i + 1] = Offset + 1;
            QuadIndices[i + 2] = Offset + 2;

            QuadIndices[i + 3] = Offset + 2;
            QuadIndices[i + 4] = Offset + 3;
            QuadIndices[i + 5] = Offset + 0;

            Offset += 4;
        }

        BufferSpecification indexBufferInfo  = {};
        indexBufferInfo.Usage                = EBufferUsageFlags::INDEX_BUFFER;
        indexBufferInfo.Count                = s_RendererStorage2D->MaxIndices;
        indexBufferInfo.Size                 = s_RendererStorage2D->MaxIndices * sizeof(uint32_t);
        indexBufferInfo.Data                 = QuadIndices;
        s_RendererStorage2D->QuadIndexBuffer = IndexBuffer::Create(indexBufferInfo);

        delete[] QuadIndices;
    }

    s_RendererStorage2D->TextureSlots[0] = Renderer::GetStorageData().WhiteTexture;

    // Default 2D graphics pipeline creation
    {
        auto FlatColorShader                    = ShaderLibrary::Load("FlatColor");
        s_RendererStorage2D->VertexBufferLayout = FlatColorShader->GetVertexBufferLayout();

        PipelineSpecification quadPipelineSpec = {};
        quadPipelineSpec.Name                  = "Quad2D";
        quadPipelineSpec.bDepthTest            = true;
        quadPipelineSpec.bDepthWrite           = true;
        quadPipelineSpec.DepthCompareOp        = ECompareOp::COMPARE_OP_LESS;
        quadPipelineSpec.TargetFramebuffer     = Renderer::GetStorageData().GeometryFramebuffer;
        quadPipelineSpec.Shader                = FlatColorShader;
        quadPipelineSpec.bDynamicPolygonMode   = true;

        s_RendererStorage2D->QuadPipeline = Pipeline::Create(quadPipelineSpec);
    }
    s_RendererStorage2D->QuadPipeline->GetSpecification().Shader->Set("u_Sprites", s_RendererStorage2D->TextureSlots[0]);

    for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
    {
        s_RendererStorage2D->QuadVertexBufferBase[frame] = new QuadVertex[s_RendererStorage2D->MaxVertices];

        BufferSpecification vbInfo = {};
        vbInfo.Usage               = EBufferUsageFlags::VERTEX_BUFFER | EBufferUsageFlags::TRANSFER_DST;
        s_RendererStorage2D->QuadVertexBuffers[frame].push_back(VertexBuffer::Create(vbInfo));

        s_RendererStorage2D->CurrentVertexBufferIndex[frame] = 0;
    }
}

void Renderer2D::Shutdown()
{
    GraphicsContext::Get().WaitDeviceOnFinish();

    for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
    {
        delete[] s_RendererStorage2D->QuadVertexBufferBase[frame];
        for (auto& vb : s_RendererStorage2D->QuadVertexBuffers[frame])
            vb->Destroy();
    }

    s_RendererStorage2D->QuadPipeline->Destroy();
    s_RendererStorage2D->QuadIndexBuffer->Destroy();

    delete s_RendererStorage2D;
    s_RendererStorage2D = nullptr;
}

void Renderer2D::Begin()
{
    s_RendererStorage2D->CurrentTextureSlotIndex = 1;

    s_RendererStorage2D->QuadIndexCount = 0;
    Renderer::GetStats().QuadCount      = 0;

    s_RendererStorage2D->CurrentFrameIndex = GraphicsContext::Get().GetCurrentFrameIndex();
    // TODO: Does it work??
    if (s_RendererStorage2D->CurrentVertexBufferIndex[s_RendererStorage2D->CurrentFrameIndex] <
        s_RendererStorage2D->QuadVertexBuffers[s_RendererStorage2D->CurrentFrameIndex].size())
    {
        for (auto vb = s_RendererStorage2D->QuadVertexBuffers[s_RendererStorage2D->CurrentFrameIndex].begin() + 1;
             vb != s_RendererStorage2D->QuadVertexBuffers[s_RendererStorage2D->CurrentFrameIndex].end(); ++vb)
        {
            (*vb)->Destroy();
        }
        s_RendererStorage2D->QuadVertexBuffers[s_RendererStorage2D->CurrentFrameIndex].erase(
            s_RendererStorage2D->QuadVertexBuffers[s_RendererStorage2D->CurrentFrameIndex].begin() + 1,
            s_RendererStorage2D->QuadVertexBuffers[s_RendererStorage2D->CurrentFrameIndex].end());
    }
    s_RendererStorage2D->CurrentVertexBufferIndex[s_RendererStorage2D->CurrentFrameIndex] = 0;

    s_RendererStorage2D->QuadVertexBufferPtr[s_RendererStorage2D->CurrentFrameIndex] =
        s_RendererStorage2D->QuadVertexBufferBase[s_RendererStorage2D->CurrentFrameIndex];

    s_RendererStorage2D->Sprites.clear();
}

void Renderer2D::Flush()
{
    if (s_RendererStorage2D->QuadIndexCount <= 0) return;

    std::sort(s_RendererStorage2D->Sprites.begin(), s_RendererStorage2D->Sprites.end(),
              [](const Sprite& lhs, const Sprite& rhs) { return lhs.Layer > rhs.Layer; });

    if (s_RendererStorage2D->CurrentTextureSlotIndex > 1)
    {
        std::vector<Ref<Texture2D>> texturesToUpload = {};
        for (uint32_t i = 0; i < s_RendererStorage2D->CurrentTextureSlotIndex; ++i)
        {
            texturesToUpload.push_back(s_RendererStorage2D->TextureSlots[i]);
        }
        s_RendererStorage2D->QuadPipeline->GetSpecification().Shader->Set("u_Sprites", texturesToUpload);
    }

    const uint32_t DataSize = (uint32_t)((uint8_t*)s_RendererStorage2D->QuadVertexBufferPtr[s_RendererStorage2D->CurrentFrameIndex] -
                                         (uint8_t*)s_RendererStorage2D->QuadVertexBufferBase[s_RendererStorage2D->CurrentFrameIndex]);

    auto& currentVertexBufferArray = s_RendererStorage2D->QuadVertexBuffers[s_RendererStorage2D->CurrentFrameIndex];
    if (s_RendererStorage2D->CurrentVertexBufferIndex[s_RendererStorage2D->CurrentFrameIndex] >= currentVertexBufferArray.size())
    {
        BufferSpecification vbInfo = {};
        vbInfo.Usage               = EBufferUsageFlags::VERTEX_BUFFER | EBufferUsageFlags::TRANSFER_DST;

        currentVertexBufferArray.push_back(VertexBuffer::Create(vbInfo));
    }

    static auto& rs = Renderer::GetStorageData();
    rs.UploadHeap->SetData(s_RendererStorage2D->QuadVertexBufferBase[s_RendererStorage2D->CurrentFrameIndex], DataSize);

    auto& vertexBuffer = currentVertexBufferArray[s_RendererStorage2D->CurrentVertexBufferIndex[s_RendererStorage2D->CurrentFrameIndex]];
    vertexBuffer->SetStagedData(rs.UploadHeap, DataSize);

    rs.GeometryFramebuffer[rs.CurrentFrame]->BeginPass(rs.RenderCommandBuffer[rs.CurrentFrame]);

    Renderer::DrawQuad(s_RendererStorage2D->QuadPipeline, vertexBuffer, s_RendererStorage2D->QuadIndexBuffer,
                       s_RendererStorage2D->QuadIndexCount, &s_RendererStorage2D->PushConstants);

    rs.GeometryFramebuffer[rs.CurrentFrame]->EndPass(rs.RenderCommandBuffer[rs.CurrentFrame]);

    ++Renderer::GetStats().DrawCalls;
    ++s_RendererStorage2D->CurrentVertexBufferIndex[s_RendererStorage2D->CurrentFrameIndex];
}

void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
{
    DrawQuad(glm::vec3(position, 0.0f), size, color);
}

void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
{
    DrawRotatedQuad(position, size, glm::vec3(0.0f), color);
}

void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& rotation, const glm::vec4& color)
{
    DrawRotatedQuad(glm::vec3(position, 0.0f), size, rotation, color);
}

void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation, const glm::vec4& color)
{
    if (s_RendererStorage2D->QuadIndexCount >= s_RendererStorage2D->MaxIndices) FlushAndReset();

    const auto transform = glm::translate(glm::mat4(1.0f), position) *
                           glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0, 0, 1)) *
                           glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

    DrawQuadInternal(transform, color, 0.0f);
}

void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& rotation, const Ref<Texture2D>& texture,
                          const glm::vec4& color)
{
    DrawQuad(glm::vec3(position, 0.0f), size, rotation, texture, color);
}

void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation, const Ref<Texture2D>& texture,
                          const glm::vec4& color)
{
    if (s_RendererStorage2D->QuadIndexCount >= s_RendererStorage2D->MaxIndices) FlushAndReset();

    float TextureId = 0.0f;
    for (uint32_t i = 1; i < s_RendererStorage2D->CurrentTextureSlotIndex; ++i)
    {
        if (s_RendererStorage2D->TextureSlots[i] == texture)
        {
            TextureId = (float)i;
            break;
        }
    }

    if (TextureId == 0.0f)
    {
        if (s_RendererStorage2D->CurrentTextureSlotIndex + 1 >= RendererStorage2D::MaxTextureSlots) FlushAndReset();

        s_RendererStorage2D->TextureSlots[s_RendererStorage2D->CurrentTextureSlotIndex] = texture;
        TextureId = (float)s_RendererStorage2D->CurrentTextureSlotIndex;
        ++s_RendererStorage2D->CurrentTextureSlotIndex;
    }

    const auto Transform = glm::translate(glm::mat4(1.0f), position) *
                           glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0, 0, 1)) *
                           glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
    DrawQuadInternal(Transform, color, TextureId);
}

void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color)
{
    DrawQuadInternal(transform, color, 0.0f);
}

void Renderer2D::DrawTexturedQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture,
                                  const glm::vec4& InBlendColor)
{
    DrawTexturedQuad(glm::vec3(position, 0.0f), size, texture, InBlendColor);
}

void Renderer2D::DrawTexturedQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture,
                                  const glm::vec4& InBlendColor)
{
    if (s_RendererStorage2D->QuadIndexCount >= s_RendererStorage2D->MaxIndices) FlushAndReset();

    float TextureId = 0.0f;
    for (uint32_t i = 1; i < s_RendererStorage2D->CurrentTextureSlotIndex; ++i)
    {
        if (s_RendererStorage2D->TextureSlots[i] == texture)
        {
            TextureId = (float)i;
            break;
        }
    }

    if (TextureId == 0.0f)
    {
        if (s_RendererStorage2D->CurrentTextureSlotIndex + 1 >= RendererStorage2D::MaxTextureSlots) FlushAndReset();

        s_RendererStorage2D->TextureSlots[s_RendererStorage2D->CurrentTextureSlotIndex] = texture;
        TextureId = (float)s_RendererStorage2D->CurrentTextureSlotIndex;
        ++s_RendererStorage2D->CurrentTextureSlotIndex;
    }

    const auto Transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
    DrawQuadInternal(Transform, InBlendColor, TextureId);
}

void Renderer2D::DrawTexturedQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& textureAtlas,
                                  const glm::vec2& spriteCoords, const glm::vec2& spriteSize)
{
    DrawTexturedQuad(glm::vec3(position, 0.0f), size, glm::vec3(0.0f), textureAtlas, spriteCoords, spriteSize);
}

void Renderer2D::DrawTexturedQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& textureAtlas,
                                  const glm::vec2& spriteCoords, const glm::vec2& spriteSize)
{
    DrawTexturedQuad(position, size, glm::vec3(0.0f), textureAtlas, spriteCoords, spriteSize);
}

void Renderer2D::DrawTexturedQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& rotation,
                                  const Ref<Texture2D>& textureAtlas, const glm::vec2& spriteCoords, const glm::vec2& spriteSize)
{
    DrawTexturedQuad(glm::vec3(position, 0.0f), size, rotation, textureAtlas, spriteCoords, spriteSize);
}

void Renderer2D::DrawTexturedQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation,
                                  const Ref<Texture2D>& textureAtlas, const glm::vec2& spriteCoords, const glm::vec2& spriteSize)
{
    if (s_RendererStorage2D->QuadIndexCount >= s_RendererStorage2D->MaxIndices) FlushAndReset();

    float textureID = 0.0f;
    for (uint32_t i = 1; i <= s_RendererStorage2D->CurrentTextureSlotIndex; ++i)
    {
        if (s_RendererStorage2D->TextureSlots[i] == textureAtlas)
        {
            textureID = (float)i;
            break;
        }
    }

    if (textureID == 0.0f)
    {
        s_RendererStorage2D->TextureSlots[s_RendererStorage2D->CurrentTextureSlotIndex] = textureAtlas;
        textureID = (float)s_RendererStorage2D->CurrentTextureSlotIndex;
    }

    if (s_RendererStorage2D->CurrentTextureSlotIndex + 1 >= RendererStorage2D::MaxTextureSlots)
        FlushAndReset();
    else
        ++s_RendererStorage2D->CurrentTextureSlotIndex;

    const glm::vec2 spriteSheetSize = glm::vec2(textureAtlas->GetWidth(), textureAtlas->GetHeight());
    const glm::vec2 texCoords[]     = {
        {(spriteCoords.x * spriteSize.x) / spriteSheetSize.x, (spriteCoords.y * spriteSize.y) / spriteSheetSize.y},              //
        {((spriteCoords.x + 1) * spriteSize.x) / spriteSheetSize.x, (spriteCoords.y * spriteSize.y) / spriteSheetSize.y},        //
        {((spriteCoords.x + 1) * spriteSize.x) / spriteSheetSize.x, ((spriteCoords.y + 1) * spriteSize.y) / spriteSheetSize.y},  //
        {spriteCoords.x * spriteSize.x / spriteSheetSize.x, ((spriteCoords.y + 1) * spriteSize.y) / spriteSheetSize.y}           //
    };

    const glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1, 0, 0)) *  // rotation around X
                                     glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0, 1, 0)) *  // around Y
                                     glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0, 0, 1));   // around Z
    const glm::mat4 transform =
        glm::translate(glm::mat4(1.0f), position) * rotationMatrix * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
    for (uint32_t i = 0; i < 4; ++i)
    {
        s_RendererStorage2D->QuadVertexBufferPtr[s_RendererStorage2D->CurrentFrameIndex]->Position =
            transform * s_RendererStorage2D->QuadVertexPositions[i];
        s_RendererStorage2D->QuadVertexBufferPtr[s_RendererStorage2D->CurrentFrameIndex]->Color     = glm::vec4(1.0f);
        s_RendererStorage2D->QuadVertexBufferPtr[s_RendererStorage2D->CurrentFrameIndex]->TexCoord  = texCoords[i];
        s_RendererStorage2D->QuadVertexBufferPtr[s_RendererStorage2D->CurrentFrameIndex]->TextureId = textureID;
        ++s_RendererStorage2D->QuadVertexBufferPtr[s_RendererStorage2D->CurrentFrameIndex];
    }

    s_RendererStorage2D->QuadIndexCount += 6;
    s_RendererStorage2D->PushConstants.TransformMatrix = s_RendererStorage2D->CameraProjectionMatrix;
    ++Renderer::GetStats().QuadCount;
}

void Renderer2D::DrawQuadInternal(const glm::mat4& transform, const glm::vec4& blendColor, const float textureID)
{
    for (uint32_t i = 0; i < 4; ++i)
    {
        s_RendererStorage2D->QuadVertexBufferPtr[s_RendererStorage2D->CurrentFrameIndex]->Position =
            transform * s_RendererStorage2D->QuadVertexPositions[i];
        s_RendererStorage2D->QuadVertexBufferPtr[s_RendererStorage2D->CurrentFrameIndex]->Color     = blendColor;
        s_RendererStorage2D->QuadVertexBufferPtr[s_RendererStorage2D->CurrentFrameIndex]->TexCoord  = RendererStorage2D::TextureCoords[i];
        s_RendererStorage2D->QuadVertexBufferPtr[s_RendererStorage2D->CurrentFrameIndex]->TextureId = textureID;
        ++s_RendererStorage2D->QuadVertexBufferPtr[s_RendererStorage2D->CurrentFrameIndex];
    }

    s_RendererStorage2D->QuadIndexCount += 6;

    s_RendererStorage2D->PushConstants.TransformMatrix = s_RendererStorage2D->CameraProjectionMatrix;
    s_RendererStorage2D->PushConstants.Data            = blendColor;

    ++Renderer::GetStats().QuadCount;
}

void Renderer2D::FlushAndReset()
{
    Flush();

    for (uint32_t i = 1; i <= s_RendererStorage2D->CurrentTextureSlotIndex; ++i)
        s_RendererStorage2D->TextureSlots[i] = nullptr;

    s_RendererStorage2D->CurrentTextureSlotIndex = 1;
    s_RendererStorage2D->QuadIndexCount          = 0;
    s_RendererStorage2D->QuadVertexBufferPtr[s_RendererStorage2D->CurrentFrameIndex] =
        s_RendererStorage2D->QuadVertexBufferBase[s_RendererStorage2D->CurrentFrameIndex];
}

}  // namespace Gauntlet