#pragma once

#include "Gauntlet/Core/Core.h"
#include "Gauntlet/Scene/Entity.h"

namespace Gauntlet
{

class Scene;

class SceneHierarchyPanel final : private Uncopyable, private Unmovable
{
  public:
    SceneHierarchyPanel() = default;
    SceneHierarchyPanel(const Ref<Scene>& InContext);
    ~SceneHierarchyPanel() = default;

    void SetContext(const Ref<Scene>& InContext);

    void OnImGuiRender();

  private:
    Ref<Scene> m_Context;
    Entity m_SelectionContext;

    void DrawEntityNode(Entity InEntity);
    void ShowComponents(Entity InEntity);
};

}  // namespace Gauntlet