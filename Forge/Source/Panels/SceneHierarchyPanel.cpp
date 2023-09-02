#include "SceneHierarchyPanel.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "Gauntlet/Scene/Scene.h"
#include "Gauntlet/Scene/Entity.h"
#include "Gauntlet/Scene/Components.h"

namespace Gauntlet
{
static void DrawVec3Control(const std::string& label, glm::vec3& values, const float resetValue = 0.0f, const float columnWidth = 100.0f)
{
    ImGuiIO& io   = ImGui::GetIO();
    auto BoldFont = io.Fonts->Fonts[1];

    ImGui::PushID(label.data());
    ImGui::Columns(2);  // First for label, second for values

    ImGui::SetColumnWidth(0, columnWidth);  // Label width
    ImGui::Text(label.data());
    ImGui::NextColumn();

    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0f, 0.0f});

    const float LineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2 ButtonSize      = ImVec2{LineHeight + 3.0f, LineHeight};

    ImGui::PushStyleColor(ImGuiCol_Button, {0.8f, 0.1f, 0.15f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.9f, 0.2f, 0.2f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.8f, 0.1f, 0.15f, 1.0f});

    ImGui::PushFont(BoldFont);
    if (ImGui::Button("X", ButtonSize)) values.x = resetValue;
    ImGui::PopFont();

    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##X", &values.x, 0.05f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, {0.1f, 0.8f, 0.15f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.2f, 0.9f, 0.2f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.1f, 0.8f, 0.15f, 1.0f});

    ImGui::PushFont(BoldFont);
    if (ImGui::Button("Y", ButtonSize)) values.y = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Y", &values.y, 0.05f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, {0.15f, 0.1f, 0.8f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.2f, 0.2f, 0.9f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.15f, 0.1f, 0.8f, 1.0f});

    ImGui::PushFont(BoldFont);
    if (ImGui::Button("Z", ButtonSize)) values.z = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Z", &values.z, 0.05f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PopStyleVar();
    ImGui::Columns(1);
    ImGui::PopID();
}

template <typename T, typename UIFunction> static void DrawComponent(const std::string& label, Entity entity, UIFunction&& uiFunction)
{
    if (!entity.HasComponent<T>()) return;

    const ImGuiTreeNodeFlags TreeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                             ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding |
                                             ImGuiTreeNodeFlags_AllowItemOverlap;

    ImVec2 ContentRegionAvailable = ImGui::GetContentRegionAvail();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4, 4});
    const float LineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImGui::Separator();
    const bool bIsOpened = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), TreeNodeFlags, label.data());
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
        auto& Component = entity.GetComponent<T>();
        uiFunction(Component);

        ImGui::TreePop();
    }

    if (bIsRemoved) entity.RemoveComponent<T>();
}

SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context) : m_Context(context) {}

void SceneHierarchyPanel::SetContext(const Ref<Scene>& context)
{
    m_Context          = context;
    m_SelectionContext = {};
}

void SceneHierarchyPanel::OnImGuiRender()
{
    ImGui::Begin("Outliner");

    if (m_Context)
    {
        m_Context->m_Registry.each(
            [&](auto entityID)
            {
                Entity entity{entityID, m_Context.get()};
                DrawEntityNode(entity);
            });

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

void SceneHierarchyPanel::DrawEntityNode(Entity entity)
{
    auto& tag = entity.GetComponent<TagComponent>();

    ImGuiTreeNodeFlags SelectionFlag = m_SelectionContext == entity ? ImGuiTreeNodeFlags_Selected : 0;
    const ImGuiTreeNodeFlags flags   = SelectionFlag | (ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth);
    bool bIsOpened                   = ImGui::TreeNodeEx((void*)(uint64_t)entity, flags, tag.Tag.data());

    if (ImGui::IsItemClicked())
    {
        m_SelectionContext = entity;
    }

    bool bIsEntityDeleted{false};
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Delete Entity")) bIsEntityDeleted = true;

        ImGui::EndPopup();
    }

    if (bIsOpened)
    {
        // Recursive open childs here
        ImGui::TreePop();
    }

    if (bIsEntityDeleted)
    {
        m_Context->DestroyEntity(entity);
        if (m_SelectionContext == entity) m_SelectionContext = {};
    }
}

void SceneHierarchyPanel::ShowComponents(Entity entity)
{
    if (entity.HasComponent<TagComponent>())
    {
        auto& tag = entity.GetComponent<TagComponent>().Tag;

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

        if (ImGui::MenuItem("Camera"))
        {
            m_SelectionContext.AddComponent<CameraComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Mesh"))
        {
            m_SelectionContext.AddComponent<MeshComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Point Light"))
        {
            m_SelectionContext.AddComponent<PointLightComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Directional Light"))
        {
            m_SelectionContext.AddComponent<DirectionalLightComponent>();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    ImGui::PopItemWidth();

    DrawComponent<TransformComponent>("Transform", entity,
                                      [](auto& tc)
                                      {
                                          DrawVec3Control("Translation", tc.Translation);
                                          DrawVec3Control("Rotation", tc.Rotation);
                                          DrawVec3Control("Scale", tc.Scale);
                                      });

    DrawComponent<SpriteRendererComponent>("SpriteRenderer", entity, [](auto& spc) { ImGui::ColorEdit4("Color", &spc.Color.r); });

    DrawComponent<PointLightComponent>("PointLight", entity,
                                       [](auto& lc)
                                       {
                                           ImGui::Separator();
                                           ImGui::Text("LightColor");
                                           ImGui::ColorPicker3("Color", (float*)&lc.Color);

                                           ImGui::Separator();
                                           ImGui::Text("Phong Model Settings");
                                           ImGui::DragFloat("Ambient", &lc.AmbientSpecularShininess.x, 0.05f, 0.0f, 1.0f, "%.2f");
                                           ImGui::DragFloat("Specular", &lc.AmbientSpecularShininess.y, 0.05f, 0.0f, FLT_MAX, "%.2f");
                                           ImGui::DragFloat("Shininess", &lc.AmbientSpecularShininess.z, 1.0f, 1.0f, 256.0f, "%.2f");

                                           ImGui::Separator();
                                           ImGui::Text("Attenuation");
                                           ImGui::DragFloat("Constant", &lc.CLQ.x, 0.005f, 0.0f, 5.0f, "%.2f");
                                           ImGui::DragFloat("Linear", &lc.CLQ.y, 0.0005f, 0.0f, 5.0f, "%.6f");
                                           ImGui::DragFloat("Quadratic", &lc.CLQ.z, 0.0005f, 0.0f, 5.0f, "%.6f");
                                       });

    DrawComponent<DirectionalLightComponent>("DirectionalLightComponent", entity,
                                             [](auto& dlc)
                                             {
                                                 ImGui::Separator();
                                                 DrawVec3Control("Direction", dlc.Direction);

                                                 ImGui::Separator();
                                                 ImGui::Text("LightColor");
                                                 ImGui::ColorPicker3("Color", (float*)&dlc.Color);

                                                 ImGui::Separator();
                                                 ImGui::Text("Phong Model Settings");
                                                 ImGui::DragFloat("Ambient", &dlc.AmbientSpecularShininess.x, 0.05f, 0.0f, 1.0f, "%.2f");
                                                 ImGui::DragFloat("Specular", &dlc.AmbientSpecularShininess.y, 0.05f, 0.0f, FLT_MAX,
                                                                  "%.2f");
                                                 ImGui::DragFloat("Shininess", &dlc.AmbientSpecularShininess.z, 1.0f, 1.0f, 256.0f, "%.2f");
                                             });

    DrawComponent<MeshComponent>("Mesh", entity,
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