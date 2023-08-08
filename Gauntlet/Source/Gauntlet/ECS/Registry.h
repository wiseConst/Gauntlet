#pragma once

#include <assert.h>
#include <memory>

#include "Entity.h"
#include "ComponentPool.h"

/* ECS made using these articles:
 * https://www.david-colson.com/2020/02/09/making-a-simple-ecs.html
 *
 */

class Registry final  // : private Uncopyable, private Unmovable
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

    template <typename T> T* AssignComponent(const Entity InEntity)
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
                assert(!bDoesEntityHasComponent);
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
        assert(Component);

        // Zeroing values for T
        memset(Component, 0, sizeof(T));
        return Component;
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
        assert(Component);

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
        m_FreeEntities.push_back(InEntity);
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
        assert(m_ComponentID < s_MaxComponents);
        return m_ComponentID++;
    }

    template <typename T> const std::string GetComponentName() const
    {
        // Simply extracting T's name
        const auto ComponentStructName = std::string(typeid(T).name());
        const auto CutPosition         = ComponentStructName.find_first_of(" ");  // our case is "struct T", we cut "struct "s
        assert(CutPosition != std::string::npos);

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

        assert(Index != UINT32_MAX);
        return Index;
    }

    bool IsEntityValid(const Entity InEntity) { return m_Entities[InEntity].Handle != UINT32_MAX; }
};

template <typename... ComponentTypes> class SceneView final  // : private Uncopyable, private Unmovable
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

/*
template<typename... ComponentTypes>
class SceneView
{
public:
    SceneView(Scene& scene) : m_Scene(&scene)
    {
        if (sizeof... (ComponentTypes) == 0)
        {
            all = true;
        }
        else
        {
            // Unpack the template parameters into an initializer list
            int componentIds[] = { 0, GetId<ComponentTypes>() ... };
            for (int i = 1; i < (sizeof...(ComponentTypes) + 1); i++)
                componentMask.set(componentIds[i]);
        }
    }

    struct Iterator
    {
        Iterator(Scene* pScene, EntityID index, ComponentMask mask, bool all)
            : m_Scene(pScene), index(index), mask(mask), all(all) {}

        EntityID operator*() const
        {
            return GetEntityID(()->m_Entities[index].ID);
        }

        bool operator==(const Iterator& other) const
        {
            return index == other.index || index == m_Scene->m_Entities.size();
        }

        bool operator!=(const Iterator& other) const
        {
            return index != other.index && index != m_Scene->m_Entities.size();
        }

        Iterator& operator++()
        {
            do
            {
                index++;
            } while (index < m_Scene->m_Entities.size() && !ValidIndex());
            return *this;
        }

        EntityID index;
        Scene* m_Scene;
        ComponentMask mask;
        bool all{ false };
    };

    bool ValidIndex()
    {
        return
            // It's a valid entity ID
            IsEntityValid(m_Scene->m_Entities[index].Handle) &&
            // It has the correct component mask
            (all || mask == (mask & m_Scene->m_Entities[index].mask));
    }

    const Iterator begin() const
    {
        int firstIndex = 0;
        while (firstIndex < m_Scene->m_Entities.size() &&
            (componentMask != (componentMask & m_Scene->m_Entities[firstIndex].mask)
                || !IsEntityValid(m_Scene->m_Entities[firstIndex].id)))
        {
            firstIndex++;
        }
        return Iterator(pScene, firstIndex, componentMask, all);
    }

    const Iterator end() const
    {
        return Iterator(pScene, EntityIndex(pScene->entities.size()), componentMask, all);
    }
private:
    Scene* pScene{ nullptr };
    ComponentMask componentMask;
    bool all{ false };

};*/