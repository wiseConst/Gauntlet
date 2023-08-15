#pragma once

#include "Gauntlet/Core/Core.h"
#include "Gauntlet/ECS/GRECS.h"
#include "Scene.h"

namespace Gauntlet
{

class Entity final
{

  public:
    Entity()  = default;
    ~Entity() = default;

    Entity(GRECS::Entity InEntityHandle, Scene* InScene);

    // TODO: Manage custom params to constructor
    template <typename T /*, typename... Args*/> T& AddComponent(/*Args&&... InArgs*/)
    {
        GNT_ASSERT(!HasComponent<T>(), "Entity already has the component!");

        return m_Scene->m_Registry.AssignComponent<T>(m_EntityHandle);
    }

    template <typename T> T& GetComponent()
    {
        GNT_ASSERT(HasComponent<T>(), "Entity doesn't have the component!");

        T* Component = m_Scene->m_Registry.GetComponent<T>(m_EntityHandle);
        return *Component;
    }

    template <typename T> bool HasComponent() { return m_Scene->m_Registry.GetComponent<T>(m_EntityHandle); }

    template <typename T> void RemoveComponent()
    {
        if (typeid(T) == typeid(MeshComponent))
        {
            // Temporary until I move every destroy func in destructor
            auto& mc = GetComponent<MeshComponent>();
            mc.Mesh->Destroy();
        }

        m_Scene->m_Registry.RemoveComponent<T>(m_EntityHandle);
    }

    FORCEINLINE operator uint32_t() const { return (uint32_t)m_EntityHandle; }

    FORCEINLINE bool operator==(const Entity& that) { return m_EntityHandle == that.m_EntityHandle && m_Scene == that.m_Scene; }
    FORCEINLINE bool operator!=(const Entity& that) { return !(*this == that); }
    FORCEINLINE operator GRECS::Entity() { return m_EntityHandle; }

    FORCEINLINE bool IsValid() const { return m_Scene && m_Scene->m_Registry.IsEntityValid(m_EntityHandle); }

  private:
    GRECS::Entity m_EntityHandle{0};
    Scene* m_Scene = nullptr;  // TODO: Make WeakPtr wrapper
};

}  // namespace Gauntlet