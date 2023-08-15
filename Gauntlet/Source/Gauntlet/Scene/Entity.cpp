#include "GauntletPCH.h"
#include "Entity.h"

namespace Gauntlet
{
Entity::Entity(GRECS::Entity InEntityHandle, Scene* InScene) : m_EntityHandle(InEntityHandle), m_Scene(InScene) {}

}  // namespace Gauntlet