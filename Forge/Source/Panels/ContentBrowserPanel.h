#pragma once

#include "Gauntlet/Core/Core.h"

#include <filesystem>

namespace Gauntlet
{

class Texture2D;

class ContentBrowserPanel final : private Uncopyable, private Unmovable
{
  public:
    ContentBrowserPanel();
    ~ContentBrowserPanel();

    void OnImGuiRender();

  private:
    std::filesystem::path m_CurrentDirectory;

    Ref<Texture2D> m_DirectoryIcon{nullptr};
    Ref<Texture2D> m_AudioIcon{nullptr};
    Ref<Texture2D> m_ImageIcon{nullptr};
    Ref<Texture2D> m_TextFileIcon{nullptr};
    Ref<Texture2D> m_DefaultFileIcon{nullptr};
};

}  // namespace Gauntlet