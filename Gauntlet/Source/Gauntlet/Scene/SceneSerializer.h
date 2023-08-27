#pragma once

#include "Gauntlet/Core/Core.h"

namespace Gauntlet
{

class Scene;

class SceneSerializer final : private Uncopyable, private Unmovable
{
  public:
    SceneSerializer(Ref<Scene>& scene);

    void Serialize(const std::string& filePath);
    void SerializeRuntime(const std::string& filePath);

    bool Deserialize(const std::string& filePath);
    bool DeserializeRuntime(const std::string& filePath);

  private:
    Ref<Scene>& m_Scene;
};

}  // namespace Gauntlet