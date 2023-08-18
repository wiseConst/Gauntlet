#pragma once

#include "Gauntlet/Core/Core.h"
#include "Gauntlet/ECS/GRECS.h"
#include "Components.h"

namespace Gauntlet
{
class Entity;
class SceneHierarchyPanel;

class Scene final : private Uncopyable, private Unmovable
{
  public:
    Scene(const std::string& Name = "Default");
    ~Scene();

    Entity CreateEntity(const std::string& Name = "NONE");
    void DestroyEntity(Entity InEntity);

    void OnUpdate(const float DeltaTime);

    FORCEINLINE const auto& GetName() const { return m_Name; }

  private:
    GRECS::Registry m_Registry;
    std::string m_Name;

    friend class Entity;
    friend class SceneHierarchyPanel;
};

}  // namespace Gauntlet