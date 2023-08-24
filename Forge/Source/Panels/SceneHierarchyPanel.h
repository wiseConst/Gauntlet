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
    SceneHierarchyPanel(const Ref<Scene>& context);
    ~SceneHierarchyPanel() = default;

    void SetContext(const Ref<Scene>& context);

    void OnImGuiRender();

  private:
    Ref<Scene> m_Context;
    Entity m_SelectionContext;

    void DrawEntityNode(Entity entity);
    void ShowComponents(Entity entity);
};

}  // namespace Gauntlet