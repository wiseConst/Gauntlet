#include "GauntletPCH.h"
#include "Scene.h"
#include "Entity.h"

#include "Gauntlet/Renderer/Renderer2D.h"
#include "Gauntlet/Renderer/Renderer.h"

namespace Gauntlet
{

Scene::Scene() {}

Scene::~Scene()
{
    for (GRECS::Entity ent : GRECS::SceneView(m_Registry))
    {
        Entity entity{ent, this};

        if (entity.HasComponent<MeshComponent>())
        {
            auto& Mesh = entity.GetComponent<MeshComponent>().Mesh;
            Mesh->Destroy();
        }
    }
}

Entity Scene::CreateEntity(const std::string& Name)
{
    Entity ent{m_Registry.CreateEntity(), this};
    auto& tag = ent.AddComponent<TagComponent>();

    GNT_ASSERT(Name.data(), "Invalid entity name!");
    strcpy(tag.Tag.data(), Name.data());

    return ent;
}

void Scene::DestroyEntity(Entity InEntity)
{
    if (InEntity.HasComponent<MeshComponent>())
    {
        auto& Mesh = InEntity.GetComponent<MeshComponent>().Mesh;
        Mesh->Destroy();
    }

    m_Registry.RemoveEntity(InEntity);  // Implicitly returns GRECS::Entity by Gauntlet::Entity impl
}

void Scene::OnUpdate(const float DeltaTime)
{
    for (GRECS::Entity ent : GRECS::SceneView(m_Registry))
    {
        Entity entity{ent, this};

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
        }
    }
}

}  // namespace Gauntlet