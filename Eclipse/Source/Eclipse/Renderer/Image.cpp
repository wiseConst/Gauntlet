#include "EclipsePCH.h"
#include "Image.h"

#include "RendererAPI.h"
#include "Eclipse/Platform/Vulkan/VulkanImage.h"

namespace Eclipse
{

namespace ImageUtils
{
stbi_uc* LoadImageFromFile(const std::string_view& InFilePath, int32_t* OutWidth, int32_t* OutHeight, int32_t* OutChannels,
                           ELoadImageType InLoadImageType)
{
    ELS_ASSERT(InFilePath.size() > 0, "File path is zero! %s", __FUNCTION__);

    stbi_set_flip_vertically_on_load(1);
    int DesiredChannels = 0;
    switch (InLoadImageType)
    {
        case ELoadImageType::GREY:
        {
            DesiredChannels = STBI_grey;
            break;
        }
        case ELoadImageType::GREY_ALPHA:
        {
            DesiredChannels = STBI_grey_alpha;
            break;
        }
        case ELoadImageType::RGB:
        {
            DesiredChannels = STBI_rgb;
            break;
        }
        case ELoadImageType::RGB_ALPHA:
        {
            DesiredChannels = STBI_rgb_alpha;
            break;
        }
    }

    auto Pixels = stbi_load(InFilePath.data(), OutWidth, OutHeight, OutChannels, DesiredChannels);
    if (!Pixels || !OutWidth || !OutHeight || !OutChannels)
    {
        const auto ErrorMessage = std::string("Failed to load image! Path: ") + std::string(InFilePath.data());
        ELS_ASSERT(false, ErrorMessage.data());
    }

    return Pixels;
}
}  // namespace ImageUtils

Ref<Image> Image::Create(const ImageSpecification& InImageSpecification)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan:
        {
            return MakeRef<VulkanImage>(InImageSpecification);
        }
        case RendererAPI::EAPI::None:
        {
            ELS_ASSERT(false, "Renderer API is none!");
            break;
        }
    }

    ELS_ASSERT(false, "Unknown RHI!");
    return nullptr;
}

}  // namespace Eclipse