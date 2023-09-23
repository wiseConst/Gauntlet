#include "GauntletPCH.h"
#include "Scene.h"
#include "Entity.h"

#include "Gauntlet/Renderer/Renderer2D.h"
#include "Gauntlet/Renderer/Renderer.h"

namespace Gauntlet
{

Scene::Scene(const std::string& name) : m_Name(name) {}

Scene::~Scene()
{
    m_Registry.each(
        [this](entt::entity entityID)
        {
            Entity entity{entityID, this};

            if (entity.HasComponent<MeshComponent>())
            {
                auto& Mesh = entity.GetComponent<MeshComponent>().Mesh;
                Mesh->Destroy();
            }
        });
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
    if (entity.HasComponent<MeshComponent>())
    {
        // Temporary until I move every destroy func in destructor
        auto& Mesh = entity.GetComponent<MeshComponent>().Mesh;
        Mesh->Destroy();
    }

    m_Registry.destroy(entity);  // Implicitly returns entt::entity by Gauntlet::Entity impl
}

void Scene::OnUpdate(const float deltaTime)
{
    for (auto entityID : m_Registry.view<TagComponent>())
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

                Renderer::SubmitMesh(Mesh, Transform);
            }

            if (entity.HasComponent<PointLightComponent>())
            {
                auto& plc = entity.GetComponent<PointLightComponent>();

                const glm::vec4 Position = glm::vec4(Transform.Translation, 1.0f);
                Renderer::AddPointLight(Position, plc.Color, plc.AmbientSpecularShininess, plc.CLQ);
            }

            if (entity.HasComponent<DirectionalLightComponent>())
            {
                auto& dlc = entity.GetComponent<DirectionalLightComponent>();

                Renderer::AddDirectionalLight(dlc.Color, Transform.Rotation, dlc.AmbientSpecularShininess);
            }
        }
    }
}

}  // namespace Gauntlet