#pragma once

#include "Gauntlet/Core/Core.h"
#include <entt/entt.hpp>
#include "Components.h"

namespace Gauntlet
{
class Entity;
class SceneHierarchyPanel;

class Scene final : private Uncopyable, private Unmovable
{
  public:
    Scene(const std::string& name = "Default");
    ~Scene();

    Entity CreateEntity(const std::string& name = "Entity");
    void DestroyEntity(Entity entity);

    void OnUpdate(const float deltaTime);

    FORCEINLINE const auto& GetName() const { return m_Name; }

  private:
    entt::registry m_Registry;
    std::string m_Name;

    // Specify classes that have public access to Scene class properties
    friend class Entity;
    friend class SceneHierarchyPanel;
};

}  // namespace Gauntlet