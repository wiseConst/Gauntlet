#include "EclipsePCH.h"
#include "Texture.h"

namespace Eclipse
{
stbi_uc* Texture::LoadImageFromFile(const std::string_view& InFilePath, int32_t* OutWidth, int32_t* OutHeight, int32_t* OutChannels,
                                    ELoadImageType InLoadImageType)

{
    ELS_ASSERT(InFilePath.data(), "File path is zero! %s", __FUNCTION__);

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
    ELS_ASSERT(Pixels && OutWidth && OutHeight && OutChannels, "Failed to load image!");

    return Pixels;
}

}  // namespace Eclipse