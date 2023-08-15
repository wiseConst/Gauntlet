#pragma once

#include "Gauntlet/Core/Core.h"
#include <vector>

#include "GRECSDefines.h"
#include "ComponentPool.h"

/* Gauntlet Rapid ECS made using these articles:
 * https://www.david-colson.com/2020/02/09/making-a-simple-ecs.html
 * https://austinmorlan.com/posts/entity_component_system/
 */

namespace Gauntlet
{
namespace GRECS
{

class Registry final : private Uncopyable, private Unmovable
{
  public:
    Registry()  = default;
    ~Registry() = default;

    Entity CreateEntity()
    {
        if (!m_FreeEntities.empty())
        {
            Entity FreeEntity = m_FreeEntities.back();
            m_FreeEntities.pop_back();

            m_Entities[FreeEntity].Handle = FreeEntity;
            return FreeEntity;
        }

        m_Entities.push_back({static_cast<Entity>(m_Entities.size()), ComponentMask()});

        return m_Entities.back().Handle;
    }

    // TODO: Make HasComponent function.

    template <typename T> T& AssignComponent(const Entity InEntity)
    {
        const auto ComponentName = GetComponentName<T>();
        // Preventing component duplication
        uint32_t ComponentID = UINT32_MAX;  // initially it's ID is zero what means that we haven't found the component
        for (uint32_t i = 0; i < m_ComponentPools.size(); ++i)
        {
            // Firstly check if this component does exist.
            if (strcmp(m_ComponentPools[i]->GetName().data(), ComponentName.data()) == 0)
            {
                ComponentID = i;

                // Then check if entity already has it.
                const bool bDoesEntityHasComponent = m_Entities[InEntity].Mask.test(i);
                const std::string AssertMessage    = std::string("Entity already has ") + ComponentName;
                GNT_ASSERT(!bDoesEntityHasComponent, AssertMessage.data());
                break;
            }
        }

        // Creating component pool with new type
        if (ComponentID == UINT32_MAX)
        {
            ComponentID = GetComponentID<T>();
            if (ComponentID >= m_ComponentPools.size())
            {
                m_ComponentPools.resize(m_ComponentPools.size() + 1, nullptr);
            }

            if (!m_ComponentPools[ComponentID])
            {
                m_ComponentPools[ComponentID] = std::shared_ptr<ComponentPool>(new ComponentPool(sizeof(T), ComponentName));
            }
        }

        // Set which entity has which component type
        m_Entities[InEntity].Mask.set(ComponentID);

        // Retrieving T
        T* Component = static_cast<T*>(m_ComponentPools[ComponentID]->GetComponent(InEntity));
        GNT_ASSERT(Component, "Assigned component is null, probably some shit is in component pool!");

        // Zeroing values for T
        memset(Component, 0, sizeof(T));
        return *Component;
    }

    template <typename T> T* GetComponent(const Entity InEntity)
    {
        // Find component id
        const auto ComponentName = GetComponentName<T>();
        uint32_t ComponentID     = UINT32_MAX;  // initially it's ID is zero what means that we haven't found the component
        for (uint32_t i = 0; i < m_ComponentPools.size(); ++i)
        {
            if (strcmp(m_ComponentPools[i]->GetName().data(), ComponentName.data()) == 0)
            {
                ComponentID = i;
                break;
            }
        }

        // We neither found the component nor entity doesn't have it
        if (ComponentID == UINT32_MAX || ComponentID != UINT32_MAX && !m_Entities[InEntity].Mask.test(ComponentID)) return nullptr;

        T* Component = static_cast<T*>(m_ComponentPools[ComponentID]->GetComponent(InEntity));
        GNT_ASSERT(Component, "Retrieved component is not valid!");

        return Component;
    }

    void RemoveEntity(const Entity InEntity)
    {
        // Ensures you're not accessing an entity that has been deleted
        if (m_Entities[InEntity].Handle != InEntity) return;

        // Reseting entity's component data
        for (uint32_t i = 0; i < m_ComponentPools.size(); ++i)
        {
            if (m_Entities[InEntity].Mask.test(i))
            {
                memset(m_ComponentPools[i]->GetComponent(InEntity), 0, m_ComponentPools[i]->GetComponentSize());
            }
        }

        // Deleting entity's component info && pushing entity to free entities array
        m_Entities[InEntity].Mask.reset();
        m_Entities[InEntity].Handle = UINT32_MAX;
        m_FreeEntities.push_back(InEntity);
    }

    bool IsEntityValid(const Entity InEntity) { return m_Entities[InEntity].Handle != UINT32_MAX; }

    template <typename T> void RemoveComponent(const Entity InEntity)
    {
        const auto ComponentName = GetComponentName<T>();
        uint32_t ComponentID     = UINT32_MAX;  // initially it's ID is zero what means that we haven't found the component
        for (uint32_t i = 0; i < m_ComponentPools.size(); ++i)
        {
            if (strcmp(m_ComponentPools[i]->GetName().data(), ComponentName.data()) == 0)
            {
                ComponentID = i;
                break;
            }
        }

        if (ComponentID == UINT32_MAX) return;

        m_Entities[InEntity].Mask.flip(ComponentID);
    }

  private:
    struct EntityDescription
    {
        EntityDescription(Entity InHandle, const ComponentMask& InComponentMask) : Handle(InHandle), Mask(InComponentMask) {}

        Entity Handle;
        ComponentMask Mask;
    };

    std::vector<std::shared_ptr<ComponentPool>> m_ComponentPools;  // Each component type has it's own pool
    std::vector<EntityDescription> m_Entities;
    std::vector<Entity> m_FreeEntities;
    uint32_t m_ComponentID = 0;

    template <typename T> uint32_t GetComponentID()
    {
        GNT_ASSERT(m_ComponentID < s_MaxComponents, "Reached limit of maximum components! TODO: Resize or what? How do I extend it.");
        return m_ComponentID++;
    }

    template <typename T> const std::string GetComponentName() const
    {
        // Simply extracting T's name
        const auto ComponentStructName = std::string(typeid(T).name());
        const auto CutPosition         = ComponentStructName.find_last_of(":");  // our case is "struct T", we cut "struct "s
        GNT_ASSERT(CutPosition != std::string::npos, "What the fuck are you trying to do? Seems like your component doesn't have a name!");

        return ComponentStructName.substr(CutPosition + 1);
    }

    // Scene View
    template <typename... ComponentTypes> friend class SceneView;

    template <typename T> uint32_t GetComponentIndexInPool()
    {
        uint32_t Index           = UINT32_MAX;
        const auto ComponentName = GetComponentName<T>();
        for (uint32_t i = 0; i < m_ComponentPools.size(); ++i)
        {
            if (strcmp(m_ComponentPools[i]->GetName().data(), ComponentName.data()) == 0)
            {
                return i;
            }
        }

        GNT_ASSERT(Index != UINT32_MAX, "Component doesn't exist in pool!");
        return Index;
    }
};

template <typename... ComponentTypes> class SceneView final : private Uncopyable, private Unmovable
{
  public:
    SceneView(Registry& InRegistry) : m_Registry(InRegistry)
    {
        // We got 0 templated params what means we want overall scene view
        if (sizeof...(ComponentTypes) == 0)
        {
            m_bAllComponents = true;
            for (auto& OneEntity : m_Registry.m_Entities)
            {
                m_Entities.push_back(OneEntity.Handle);
            }
        }
        else
        {
            uint32_t ComponentIndices[] = {0, m_Registry.GetComponentIndexInPool<ComponentTypes>()...};
            for (auto& OneEntity : m_Registry.m_Entities)
            {
                bool bDoesEntityHasComponents{true};
                for (uint32_t i = 1; i < sizeof...(ComponentTypes) + 1; ++i)
                {
                    if (!m_Registry.m_Entities[OneEntity.Handle].Mask.test(ComponentIndices[i]))
                    {
                        bDoesEntityHasComponents = false;
                        break;
                    }
                }

                if (bDoesEntityHasComponents) m_Entities.push_back(OneEntity.Handle);
            }
        }
    }

    ~SceneView() = default;

    struct Iterator
    {
        Iterator(Registry* InRegistry, Entity InEntity, const std::vector<Entity> InSceneEntities)
            : m_Registry(InRegistry), m_CurrentEntity(InEntity), m_Entities(InSceneEntities)
        {
        }

        Iterator()  = default;
        ~Iterator() = default;

        bool operator==(const Iterator& other) const
        {
            return m_CurrentEntity == other.m_CurrentEntity || m_CurrentEntity == m_Entities.size();
        }

        bool operator!=(const Iterator& other) const { return !(*this == other); }

        Iterator& operator++()
        {
            do
            {
                ++m_CurrentEntity;
            } while (m_CurrentEntity < m_Entities.size() && !m_Registry->IsEntityValid(m_CurrentEntity));

            return *this;
        }

        Entity operator*() { return m_Entities[m_CurrentEntity]; }

        Entity m_CurrentEntity;
        Registry* m_Registry;
        std::vector<Entity> m_Entities;
    };

    Iterator begin()
    {
        Entity CurrentEntity = 0;
        while (CurrentEntity < m_Entities.size() && !m_Registry.IsEntityValid(CurrentEntity))
        {
            ++CurrentEntity;
        }
        return Iterator(&m_Registry, CurrentEntity, m_Entities);
    }

    Iterator end() { return Iterator(&m_Registry, m_Registry.m_Entities.size(), m_Entities); }

  private:
    Registry& m_Registry;
    bool m_bAllComponents{false};
    std::vector<Entity> m_Entities;
};
}  // namespace GRECS

}  // namespace Gauntlet