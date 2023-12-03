#include "GauntletPCH.h"
#include "Scene.h"
#include "Entity.h"

#include "Gauntlet/Renderer/Renderer2D.h"
#include "Gauntlet/Renderer/Renderer.h"

#pragma warning(disable : 4996)

namespace Gauntlet
{

Scene::Scene(const std::string& name) : m_Name(name) {}

Scene::~Scene()
{
    for (auto entityID : m_Registry.view<IDComponent>())
    {
        m_Registry.destroy(entityID);
    }
}

Entity Scene::CreateEntity(const std::string& name)
{
    return CreateEntityWithUUID(UUID(), name);
}

Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
{
    GNT_ASSERT(!name.empty(), "Entity name is empty!");

    Entity entity{m_Registry.create(), this};
    entity.AddComponent<IDComponent>(uuid);

    auto& tag = entity.AddComponent<TagComponent>();
    tag.Tag   = name;

    entity.AddComponent<TransformComponent>();

    return entity;
}

void Scene::DestroyEntity(Entity entity)
{
    m_Registry.destroy(entity);  // Implicitly returns entt::entity by Gauntlet::Entity impl
}

void Scene::OnUpdate(const float deltaTime)
{
    for (auto entityID : m_Registry.view<IDComponent>())
    {
        Entity entity{entityID, this};

        if (entity.HasComponent<TransformComponent>())
        {
            auto& Transform = entity.GetComponent<TransformComponent>();

            if (entity.HasComponent<SpriteRendererComponent>())
            {
                auto& Color = entity.GetComponent<SpriteRendererComponent>().Color;

                Renderer2D::DrawQuad(Transform, Color);
            }

            if (entity.HasComponent<MeshComponent>())
            {
                auto& Mesh = entity.GetComponent<MeshComponent>().Mesh;

                if (Mesh->IsAnimated())
                {
                    // Animate here
                }
                else
                {
                }

                Renderer::SubmitMesh(Mesh, Transform);
            }

            if (entity.HasComponent<PointLightComponent>())
            {
                auto& plc = entity.GetComponent<PointLightComponent>();

                const glm::vec4 Position = glm::vec4(Transform.Translation, 1.0f);
                Renderer::AddPointLight(Position, plc.Color, plc.Intensity, plc.bIsActive);
            }

            if (entity.HasComponent<DirectionalLightComponent>())
            {
                auto& dlc = entity.GetComponent<DirectionalLightComponent>();

                Renderer::AddDirectionalLight(dlc.Color, glm::radians(Transform.Rotation), dlc.bCastShadows, dlc.Intensity);
            }

            if (entity.HasComponent<SpotLightComponent>())
            {
                auto& slc = entity.GetComponent<SpotLightComponent>();

                Renderer::AddSpotLight(Transform.Translation, glm::radians(Transform.Rotation), slc.Color, slc.Intensity,
                                       (int32_t)slc.bIsActive, glm::cos(glm::radians(slc.CutOff)), glm::cos(glm::radians(slc.OuterCutOff)));
            }
        }
    }
}

}  // namespace Gauntlet