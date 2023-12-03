#pragma once

#include "Gauntlet/Core/Core.h"
#include "Gauntlet/Core/UUID.h"
#include "Scene.h"

namespace Gauntlet
{

class Entity final
{

  public:
    Entity()  = default;
    ~Entity() = default;

    Entity(entt::entity entity, Scene* scene);

    template <typename T> FORCEINLINE bool HasComponent() { return m_Scene->m_Registry.all_of<T>(m_EntityHandle); }

    template <typename T, typename... Args> T& AddComponent(Args&&... args)
    {
        GNT_ASSERT(!HasComponent<T>(), "Entity already has the component!");

        return m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
    }

    template <typename T> T& GetComponent()
    {
        GNT_ASSERT(HasComponent<T>(), "Entity doesn't have the component!");

        return m_Scene->m_Registry.get<T>(m_EntityHandle);
    }

    template <typename T> void RemoveComponent()
    {
        GNT_ASSERT(HasComponent<T>(), "Attempting to delete component that entity doesn't have.");

        if (typeid(T) == typeid(IDComponent) || typeid(T) == typeid(TransformComponent) || typeid(T) == typeid(TagComponent))
        {
            LOG_WARN("Attempting to delete transform/tag component! Returning...");
            return;
        }
        m_Scene->m_Registry.remove<T>(m_EntityHandle);
    }

    FORCEINLINE operator uint32_t() const { return (uint32_t)m_EntityHandle; }

    FORCEINLINE bool operator==(const Entity& that) { return m_EntityHandle == that.m_EntityHandle && m_Scene == that.m_Scene; }
    FORCEINLINE bool operator!=(const Entity& that) { return !(*this == that); }
    FORCEINLINE operator entt::entity() { return m_EntityHandle; }

    FORCEINLINE bool IsValid() { return m_Scene && m_EntityHandle != entt::null; }
    FORCEINLINE UUID GetUUID() { return GetComponent<IDComponent>().ID; }

  private:
    entt::entity m_EntityHandle{entt::null};
    Scene* m_Scene{nullptr};
};

}  // namespace Gauntlet