#pragma once

#include "Gauntlet/Core/Core.h"
#include "GRECSDefines.h"

namespace Gauntlet
{
namespace GRECS
{
class ComponentPool final
{
  public:
    ComponentPool() : m_ComponentSize(0), m_Data(nullptr) {}

    ComponentPool(std::size_t ComponentSize, const std::string& ComponentName)
        : m_ComponentName(ComponentName), m_ComponentSize(ComponentSize), m_Data(nullptr)
    {
        GNT_ASSERT(ComponentName.size() > 0, "Seems like you're trying to create component pool who's NONAME!");
        m_Data = new uint8_t[ComponentSize * s_MaxEntities];
        GNT_ASSERT(m_Data, "Failed to allocate component pool data!");
    }

    ~ComponentPool()
    {
        if (m_Data)
        {
            delete[] m_Data;
            m_Data = nullptr;
        }
    }

    void* GetComponent(const uint32_t Index)
    {
        GNT_ASSERT(Index < s_MaxEntities, "Reached entity limit!");
        return m_Data + Index * m_ComponentSize;
    }

    const std::size_t GetComponentSize() { return m_ComponentSize; }

    const bool IsValid() const { return m_ComponentSize > 0 && m_Data; }
    const auto& GetName() const { return m_ComponentName; }
    const auto& GetName() { return m_ComponentName; }

  private:
    std::string m_ComponentName;
    std::size_t m_ComponentSize;
    uint8_t* m_Data;
};
}  // namespace GRECS
}  // namespace Gauntlet