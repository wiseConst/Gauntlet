#pragma once

#include "Eclipse/Core/Core.h"
#include <stb/stb_image.h>

namespace Eclipse
{

enum class ELoadImageType : uint8_t
{
    GREY = 0,
    GREY_ALPHA,
    RGB,
    RGB_ALPHA,
};

stbi_uc* LoadImageFromFile(const std::string_view& InFilePath, int32_t* OutWidth, int32_t* OutHeight, int32_t* OutChannels,
                           ELoadImageType InLoadImageType = ELoadImageType::RGB_ALPHA);

FORCEINLINE void UnloadImage(stbi_uc* InPixels)
{
    stbi_image_free(InPixels);
}

}  // namespace Eclipse