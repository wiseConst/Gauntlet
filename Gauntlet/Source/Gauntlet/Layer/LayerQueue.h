#pragma once

#include "Gauntlet/Core/Core.h"
#include "Layer.h"

#include <queue>

namespace Gauntlet
{

class LayerQueue final : private Uncopyable, private Unmovable
{
  public:
    LayerQueue()  = default;
    ~LayerQueue() = default;

    FORCEINLINE void Init()
    {
        static bool s_bIsInitialized{false};
        GNT_ASSERT(!s_bIsInitialized, "Layer queue is already initialized!");

        for (auto* Layer : m_Queue)
        {
            GNT_ASSERT(Layer, "Seems like you've enqueued layer later, but now it's not valid!");

            Layer->OnAttach();
        }
    }

    FORCEINLINE void OnUpdate(const float DeltaTime)
    {
        for (auto* Layer : m_Queue)
        {
            if (!Layer) continue;

            Layer->OnUpdate(DeltaTime);
        }
    }

    FORCEINLINE void OnImGuiRender()
    {
        for (auto* Layer : m_Queue)
        {
            if (!Layer) continue;

            Layer->OnImGuiRender();
        }
    }

    FORCEINLINE void Enqueue(Layer* InLayer)
    {
        GNT_ASSERT(InLayer, "Layer you want to enqueue is null!");

        m_Queue.emplace_back(InLayer);
    }

    FORCEINLINE void Destroy()
    {
        for (auto* Layer : m_Queue)
        {
            if (!Layer) continue;

            Layer->OnDetach();

            delete Layer;
            Layer = nullptr;
        }
    }

    FORCEINLINE const auto begin() const { return m_Queue.begin(); }
    FORCEINLINE const auto end() const { return m_Queue.end(); }

    FORCEINLINE auto begin() { return m_Queue.begin(); }
    FORCEINLINE auto end() { return m_Queue.end(); }

  private:
    std::vector<Layer*> m_Queue;
};
}  // namespace Gauntlet