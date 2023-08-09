#include "GauntletPCH.h"
#include "Image.h"

#include "RendererAPI.h"
#include "Gauntlet/Platform/Vulkan/VulkanImage.h"

namespace Gauntlet
{

namespace ImageUtils
{
stbi_uc* LoadImageFromFile(const std::string_view& InFilePath, int32_t* OutWidth, int32_t* OutHeight, int32_t* OutChannels,
                           const bool InbFlipOnLoad, ELoadImageType InLoadImageType)
{
    GNT_ASSERT(InFilePath.size() > 0, "File path is zero! %s", __FUNCTION__);

    if (InbFlipOnLoad) stbi_set_flip_vertically_on_load(1);
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
        GNT_ASSERT(false, ErrorMessage.data());
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
            GNT_ASSERT(false, "Renderer API is none!");
            break;
        }
    }

    GNT_ASSERT(false, "Unknown RHI!");
    return nullptr;
}

}  // namespace Gauntlet