#include "EclipsePCH.h"
#include "VulkanTexture.h"

#include "VulkanCore.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"

#include "Eclipse/Core/Application.h"

namespace Eclipse
{

// VulkanTexture2D::VulkanTexture2D(const std::string_view& TextureFilePath) : Texture2D(TextureFilePath), m_FilePath(TextureFilePath)
//{
//     Application::Get().GetThreadPool()->Enqueue(&VulkanTexture2D::LoadTexture, this);
// }
//
// void VulkanTexture2D::Destroy()
//{
//     m_Image->Destroy();
// }
//
// void VulkanTexture2D::LoadTexture()
//{
//     AllocatedBuffer StagingBuffer = {};
//     LoadImageFromDisk(m_FilePath, StagingBuffer);
//     // CreateImage(&m_Image, m_ImageWidth, m_ImageHeight, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
//
//     // TransitionImageLayout(m_Image.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
//     CopyBufferDataToImage(StagingBuffer);
//     // TransitionImageLayout(m_Image.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
//
//     VulkanContext& context = (VulkanContext&)VulkanContext::Get();
//     ELS_ASSERT(context.GetDevice()->GetLogicalDevice() && context.GetDevice()->GetPhysicalDevice(), "Rendering device is invalid!");
//
//     context.GetAllocator()->DestroyBuffer(StagingBuffer.Buffer, StagingBuffer.Allocation);
//
//     // VulkanImage::CreateImageView(context.GetDevice()->GetLogicalDevice(), m_Image.Image, &m_Image.ImageView, VK_FORMAT_R8G8B8A8_SRGB,
//     // VK_IMAGE_ASPECT_COLOR_BIT);
//     CreateSampler();
// }

VulkanTexture2D::VulkanTexture2D(const std::string_view& TextureFilePath) {}

void VulkanTexture2D::Destroy() {}

}  // namespace Eclipse