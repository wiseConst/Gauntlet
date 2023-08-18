#include "SceneHierarchyPanel.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "Gauntlet/Scene/Scene.h"
#include "Gauntlet/Scene/Entity.h"
#include "Gauntlet/Scene/Components.h"

namespace Gauntlet
{
static void DrawVec3Control(const std::string& InLabel, glm::vec3& InValues, const float InResetValue = 0.0f,
                            const float InColumnWidth = 100.0f)
{
    ImGuiIO& io   = ImGui::GetIO();
    auto BoldFont = io.Fonts->Fonts[1];

    ImGui::PushID(InLabel.data());
    ImGui::Columns(2);  // First for label, second for values

    ImGui::SetColumnWidth(0, InColumnWidth);  // Label width
    ImGui::Text(InLabel.data());
    ImGui::NextColumn();

    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0f, 0.0f});

    const float LineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2 ButtonSize      = ImVec2{LineHeight + 3.0f, LineHeight};

    ImGui::PushStyleColor(ImGuiCol_Button, {0.8f, 0.1f, 0.15f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.9f, 0.2f, 0.2f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.8f, 0.1f, 0.15f, 1.0f});

    ImGui::PushFont(BoldFont);
    if (ImGui::Button("X", ButtonSize)) InValues.x = InResetValue;
    ImGui::PopFont();

    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##X", &InValues.x, 0.05f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, {0.1f, 0.8f, 0.15f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.2f, 0.9f, 0.2f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.1f, 0.8f, 0.15f, 1.0f});

    ImGui::PushFont(BoldFont);
    if (ImGui::Button("Y", ButtonSize)) InValues.y = InResetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Y", &InValues.y, 0.05f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, {0.15f, 0.1f, 0.8f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.2f, 0.2f, 0.9f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.15f, 0.1f, 0.8f, 1.0f});

    ImGui::PushFont(BoldFont);
    if (ImGui::Button("Z", ButtonSize)) InValues.z = InResetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Z", &InValues.z, 0.05f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PopStyleVar();
    ImGui::Columns(1);
    ImGui::PopID();
}

template <typename T, typename UIFunction> static void DrawComponent(const std::string& InLabel, Entity InEntity, UIFunction&& InUIFunction)
{
    if (!InEntity.HasComponent<T>()) return;

    const ImGuiTreeNodeFlags TreeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                             ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding |
                                             ImGuiTreeNodeFlags_AllowItemOverlap;

    ImVec2 ContentRegionAvailable = ImGui::GetContentRegionAvail();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4, 4});
    const float LineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImGui::Separator();
    const bool bIsOpened = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), TreeNodeFlags, InLabel.data());
    ImGui::PopStyleVar();

    ImGui::SameLine(ContentRegionAvailable.x - LineHeight * 0.5f);
    if (ImGui::Button("...", ImVec2{LineHeight, LineHeight})) ImGui::OpenPopup("ComponentSettings");

    bool bIsRemoved = false;
    if (ImGui::BeginPopup("ComponentSettings"))
    {
        if (ImGui::MenuItem("Remove Component")) bIsRemoved = true;

        ImGui::EndPopup();
    }

    if (bIsOpened)
    {
        auto& Component = InEntity.GetComponent<T>();
        InUIFunction(Component);

        ImGui::TreePop();
    }

    if (bIsRemoved) InEntity.RemoveComponent<T>();
}

SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& InContext) : m_Context(InContext) {}

void SceneHierarchyPanel::SetContext(const Ref<Scene>& InContext)
{
    m_Context = InContext;
}

void SceneHierarchyPanel::OnImGuiRender()
{
    ImGui::Begin("Outliner");

    if (m_Context)
    {
        for (GRECS::Entity ent : GRECS::SceneView(m_Context->m_Registry))
        {
            Entity entity{ent, m_Context.get()};
            DrawEntityNode(entity);
        }

        // Deselection
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered()) m_SelectionContext = {};

        // Second flag specifies that popup window should only be opened if only MB_Right
        // down in the blank space(nothing's hovered)
        if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem("Create Empty Entity")) m_Context->CreateEntity("Empty Entity");

            ImGui::EndPopup();
        }
    }
    ImGui::End();

    ImGui::Begin("Details");
    if (m_SelectionContext.IsValid()) ShowComponents(m_SelectionContext);
    ImGui::End();
}

void SceneHierarchyPanel::DrawEntityNode(Entity InEntity)
{
    const auto& tag = InEntity.GetComponent<TagComponent>().Tag;

    ImGuiTreeNodeFlags SelectionFlag = m_SelectionContext == InEntity ? ImGuiTreeNodeFlags_Selected : 0;
    const ImGuiTreeNodeFlags flags   = SelectionFlag | (ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth);
    bool bIsOpened                   = ImGui::TreeNodeEx((void*)(uint64_t)InEntity, flags, tag.data());

    if (ImGui::IsItemClicked())
    {
        m_SelectionContext = InEntity;
    }

    bool bIsEntityDeleted{false};
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Delete Entity")) bIsEntityDeleted = true;

        ImGui::EndPopup();
    }

    if (bIsOpened)
    {
        // Recursive open
        ImGui::TreePop();
    }

    if (bIsEntityDeleted)
    {
        m_Context->DestroyEntity(InEntity);
        if (m_SelectionContext == InEntity) m_SelectionContext = {};
    }
}

void SceneHierarchyPanel::ShowComponents(Entity InEntity)
{
    if (InEntity.HasComponent<TagComponent>())
    {
        auto& tag = InEntity.GetComponent<TagComponent>().Tag;

        char name[256];
        memset(name, 0, sizeof(name));
        strcpy_s(name, sizeof(name), tag.data());

        if (ImGui::InputText("##Tag", name, sizeof(name)))
        {
            tag = std::string(name);
        }
    }

    ImGui::PushItemWidth(-1.0f);
    ImGui::SameLine();

    if (ImGui::Button("Add Component")) ImGui::OpenPopup("AddComponent");

    if (ImGui::BeginPopup("AddComponent"))
    {
        if (ImGui::MenuItem("Sprite Renderer"))
        {
            m_SelectionContext.AddComponent<SpriteRendererComponent>();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    ImGui::PopItemWidth();

    DrawComponent<TransformComponent>("Transform", InEntity,
                                      [](auto& tc)
                                      {
                                          DrawVec3Control("Translation", tc.Translation);
                                          DrawVec3Control("Rotation", tc.Rotation);
                                          DrawVec3Control("Scale", tc.Scale);
                                      });

    DrawComponent<SpriteRendererComponent>("SpriteRenderer", InEntity, [](auto& spc) { ImGui::ColorEdit4("Color", &spc.Color.r); });

    DrawComponent<LightComponent>("Light", InEntity,
                                  [](auto& lc)
                                  {
                                      ImGui::Separator();
                                      ImGui::ColorPicker4("Color", (float*)&lc.LightColor);

                                      ImGui::Separator();
                                      ImGui::DragFloat("Ambient", &lc.AmbientSpecularShininess.x, 0.05f, 0.0f, FLT_MAX, "%.2f");
                                      ImGui::DragFloat("Specular", &lc.AmbientSpecularShininess.y, 0.05f, 0.0f, FLT_MAX, "%.2f");
                                      ImGui::DragFloat("Shininess", &lc.AmbientSpecularShininess.z, 1.0f, 0.0f, FLT_MAX, "%.2f");
                                  });

    DrawComponent<MeshComponent>("Mesh", InEntity,
                                 [](auto& mc)
                                 {
                                     for (uint32_t i = 0; i < mc.Mesh->GetSubmeshCount(); ++i)
                                     {
                                         ImGui::Separator();
                                         ImGui::Text(mc.Mesh->GetSubmeshName(i).data());

                                         const Ref<Gauntlet::Material>& Mat = mc.Mesh->GetMaterial(i);

                                         constexpr ImVec2 ImageSize(256.0f, 256.0f);

                                         Ref<Texture2D> DiffuseTexture = Mat->GetDiffuseTexture(0);
                                         if (DiffuseTexture && DiffuseTexture->GetTextureID())
                                             ImGui::Image(DiffuseTexture->GetTextureID(), ImageSize);

                                         Ref<Texture2D> NormalMapTexture = Mat->GetNormalMapTexture(0);
                                         if (NormalMapTexture && NormalMapTexture->GetTextureID())
                                             ImGui::Image(NormalMapTexture->GetTextureID(), ImageSize);

                                         Ref<Texture2D> EmissiveTexture = Mat->GetEmissiveTexture(0);
                                         if (EmissiveTexture && EmissiveTexture->GetTextureID())
                                             ImGui::Image(EmissiveTexture->GetTextureID(), ImageSize);
                                     }
                                 });
}

}  // namespace Gauntlet