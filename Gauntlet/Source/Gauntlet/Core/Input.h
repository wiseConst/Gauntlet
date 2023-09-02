#pragma once

#include "Core.h"
#include "InputCodes.h"

namespace Gauntlet
{

class Input final : private Uncopyable, private Unmovable
{
  public:
    virtual ~Input() = default;

    static bool IsKeyPressed(KeyCode keyCode);
    static bool IsMouseButtonPressed(KeyCode button);

    static bool IsKeyReleased(KeyCode keyCode);
    static bool IsMouseButtonReleased(KeyCode button);

    static std::pair<int32_t, int32_t> GetMousePosition();
    static int32_t GetMouseX();
    static int32_t GetMouseY();

  private:
    Input() = default;
};
}  // namespace Gauntlet