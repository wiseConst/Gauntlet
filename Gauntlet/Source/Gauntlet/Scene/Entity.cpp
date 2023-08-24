#include "GauntletPCH.h"
#include "Entity.h"

namespace Gauntlet
{
Entity::Entity(entt::entity entity, Scene* scene) : m_EntityHandle(entity), m_Scene(scene) {}

}  // namespace Gauntlet