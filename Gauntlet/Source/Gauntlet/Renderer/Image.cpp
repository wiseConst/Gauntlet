#include "GauntletPCH.h"
#include "Image.h"

#include "RendererAPI.h"
#include "Gauntlet/Platform/Vulkan/VulkanImage.h"

namespace Gauntlet
{

namespace ImageUtils
{
stbi_uc* LoadImageFromFile(const std::string_view& filePath, int32_t* outWidth, int32_t* outHeight, int32_t* outChannels,
                           const bool bFlipOnLoad, ELoadImageType loadImageType)
{
    GNT_ASSERT(filePath.size() > 0, "File path is zero! %s", __FUNCTION__);

    if (bFlipOnLoad) stbi_set_flip_vertically_on_load(1);
    int DesiredChannels = 0;
    switch (loadImageType)
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

    auto Pixels = stbi_load(filePath.data(), outWidth, outHeight, outChannels, DesiredChannels);
    if (!Pixels || !outWidth || !outHeight || !outChannels)
    {
        const auto ErrorMessage = std::string("Failed to load image! Path: ") + std::string(filePath.data());
        GNT_ASSERT(false, ErrorMessage.data());
    }

    return Pixels;
}
}  // namespace ImageUtils

Ref<Image> Image::Create(const ImageSpecification& imageSpecification)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan:
        {
            return MakeRef<VulkanImage>(imageSpecification);
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

void SamplerStorage::Initialize()
{
    GNT_ASSERT(!s_Instance, "Sampler storage already created!");
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan:
        {
            s_Instance = new VulkanSamplerStorage();
            break;
        }
        case RendererAPI::EAPI::None:
        {
            LOG_ERROR("RendererAPI::EAPI::None!");
            GNT_ASSERT(false, "Unknown RendererAPI!");
            break;
        }
    }

    s_Instance->InitializeImpl();
}

void SamplerStorage::Destroy()
{
    s_Instance->DestroyImpl();

    delete s_Instance;
    s_Instance = nullptr;
}

}  // namespace Gauntlet