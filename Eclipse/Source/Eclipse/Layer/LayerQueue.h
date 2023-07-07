#pragma once

#include "Eclipse/Core/Core.h"
#include "Layer.h"

#include <queue>

namespace Eclipse
{

class LayerQueue final : private Uncopyable, private Unmovable
{
  public:
    LayerQueue() = default;

    ~LayerQueue()
    {
        for (auto* Layer : m_Queue)
        {
            if (!Layer) continue;

            Layer->OnDetach();

            delete Layer;
            Layer = nullptr;
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

    FORCEINLINE void Enqueue(Layer* InLayer)
    {
        ELS_ASSERT(InLayer, "Layer you want to enqueue is null!");

        InLayer->OnAttach();
        m_Queue.emplace_back(InLayer);
    }

    FORCEINLINE const auto begin() const { return m_Queue.begin(); }
    FORCEINLINE const auto end() const { return m_Queue.end(); }

    FORCEINLINE auto begin() { return m_Queue.begin(); }
    FORCEINLINE auto end() { return m_Queue.end(); }

  private:
    std::vector<Layer*> m_Queue;
};
}  // namespace Eclipse