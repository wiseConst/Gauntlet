#pragma once

#include <stdint.h>
#include <assert.h>

class ComponentPool final
{
  public:
    ComponentPool() : m_ComponentSize(0), m_Data(nullptr) {}

    ComponentPool(std::size_t ComponentSize, const std::string& ComponentName)
        : m_ComponentName(ComponentName), m_ComponentSize(ComponentSize), m_Data(nullptr)
    {
        assert(ComponentName.size() > 0);
        m_Data = new uint8_t[ComponentSize * s_MaxEntities];
        assert(m_Data);
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
        assert(Index < s_MaxEntities);
        return m_Data + Index * m_ComponentSize;
    }

    const std::size_t GetComponentSize() { return m_ComponentSize; }

    bool IsValid() const { return m_ComponentSize > 0 && m_Data; }
    const auto& GetName() const { return m_ComponentName; }
    const auto& GetName() { return m_ComponentName; }

  private:
    std::string m_ComponentName;
    std::size_t m_ComponentSize;
    uint8_t* m_Data;
};