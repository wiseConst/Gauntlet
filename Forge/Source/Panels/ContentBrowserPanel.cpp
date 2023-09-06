#include "ContentBrowserPanel.h"
#include <Gauntlet.h>

#include <imgui/imgui.h>

namespace Gauntlet
{
// TODO: Get it from application class, actual path setup in EditorLayer.cpp
extern const std::filesystem::path g_AssetsPath;

ContentBrowserPanel::ContentBrowserPanel() : m_CurrentDirectory(g_AssetsPath)
{
    m_DirectoryIcon   = Texture2D::Create("Resources/Icons/icons8/folder-48.png", true);
    m_AudioIcon       = Texture2D::Create("Resources/Icons/icons8/audio-file-48.png", true);
    m_ImageIcon       = Texture2D::Create("Resources/Icons/icons8/image-file-48.png", true);
    m_TextFileIcon    = Texture2D::Create("Resources/Icons/icons8/txt-48.png", true);
    m_DefaultFileIcon = Texture2D::Create("Resources/Icons/icons8/file-48.png", true);
}

ContentBrowserPanel::~ContentBrowserPanel()
{
    m_DirectoryIcon->Destroy();
    m_AudioIcon->Destroy();
    m_ImageIcon->Destroy();
    m_TextFileIcon->Destroy();
    m_DefaultFileIcon->Destroy();
}

void ContentBrowserPanel::OnImGuiRender()
{
    ImGui::Begin("Content Browser");

    if (m_CurrentDirectory != g_AssetsPath)
    {
        if (ImGui::Button("Back")) m_CurrentDirectory = m_CurrentDirectory.parent_path();
    }

    static float padding       = 16.0f;
    static float thumbnailSize = 96.0f;
    const float cellSize       = thumbnailSize + padding;

    const float panelWidth = ImGui::GetContentRegionAvail().x;
    uint32_t columnCount   = (uint32_t)panelWidth / cellSize;
    if (columnCount < 1) columnCount = 1;
    ImGui::Columns(columnCount, 0, false);

    for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
    {
        const auto path                  = directoryEntry.path();
        const std::string filenameString = path.filename().string();

        Ref<Texture2D> icon = directoryEntry.is_directory() ? m_DirectoryIcon : m_DefaultFileIcon;

        if (directoryEntry.path().extension().string() == std::string(".png") ||
            directoryEntry.path().extension().string() == std::string(".jpg"))
            icon = m_ImageIcon;
        else if (directoryEntry.path().extension().string() == std::string(".txt"))
            icon = m_TextFileIcon;

        ImGui::PushStyleColor(ImGuiCol_Button, {0, 0, 0, 0});
        ImGui::PushID(filenameString.c_str());
        ImGui::ImageButton(icon->GetTextureID(), ImVec2{thumbnailSize, thumbnailSize}, {0, 1}, {1, 0});

        if (ImGui::BeginDragDropSource())
        {
            const std::filesystem::path relativePath(path);
            const wchar_t* itemPath = relativePath.c_str();

            ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath, /* +1 for NT */ (wcslen(itemPath) + 1) * sizeof(wchar_t),
                                      ImGuiCond_Once);
            ImGui::Text("%s", filenameString.data());
            ImGui::EndDragDropSource();
        }

        ImGui::PopID();
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            if (directoryEntry.is_directory()) m_CurrentDirectory /= path.filename();
        }
        ImGui::TextWrapped(filenameString.data());

        ImGui::NextColumn();
    }

    ImGui::Columns(1);

    // TODO: Remove/Add to engine settings
    // ImGui::SliderFloat("ThumbnailSize", &thumbnailSize, 16.0f, 512.0f);
    // ImGui::SliderFloat("Padding", &padding, 0.0f, 32.0f);
    // if (padding < 1.0f) padding = 1.0f;  // Prevent imgui textures flickering

    ImGui::End();
}

}  // namespace Gauntlet